// elfrun - host-side stand-in for ESP-IDF's elf_loader, for running
// elf_loader-style ELF binaries under qemu-riscv32 in CI.
//
// Loads a shared ELF built like:
//   riscv32-esp-elf-gcc -Dmain=app_main -nostartfiles -nostdlib -fPIC
//       -shared -Wl,-e,app_main prog.c -o prog.elf
// (or emitted by rcc700), loads its PT_LOAD segments into RWX memory like the
// device RISC-V loader, applies relocations (from the SHT_RELA sections, as
// the device does), resolves imported symbols against the export table below
// — the role the firmware's symbol table plays on-device — and jumps to the
// entry point. The program's return value becomes the exit code.
//
// elfrun itself is a regular static picolibc binary (--specs=semihost.specs);
// qemu-user services the semihosting calls, so the loaded program's printf,
// malloc, open, ... all come from the toolchain's libc, not from custom code.
//
// Usage: qemu-riscv32 elfrun prog.elf [args...]

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Three places picolibc's bare-metal defaults don't fit qemu-user, fixed
// with overrides (the first via a static heap, the rest via Linux syscalls,
// which qemu-user implements regardless of semihosting):
//  - picolibc's _sbrk (malloc's backend) hands out [__heap_start,
//    __heap_end) from the linker script, which qemu-user never maps;
//    back malloc with BSS instead
//  - semihosting SYS_EXIT drops the exit status on 32-bit targets
//  - there is no other way to get executable memory for the loaded image
static char heap[8 << 20];
static long heap_used;

void *_sbrk(ptrdiff_t n)
{
    void *p = heap + heap_used;
    n = (n + 7) & ~7;
    if (heap_used + n > (long)sizeof(heap)) return (void *)-1;
    heap_used += n;
    return p;
}

void _exit(int code)
{
    register long a0 asm("a0") = code;
    register long a7 asm("a7") = 93; // SYS_exit
    asm volatile("ecall" :: "r"(a0), "r"(a7));
    __builtin_unreachable();
}

static void *mmap_rwx(long len)
{
    register long a0 asm("a0") = 0;
    register long a1 asm("a1") = len;
    register long a2 asm("a2") = 7;    // PROT_READ|PROT_WRITE|PROT_EXEC
    register long a3 asm("a3") = 0x22; // MAP_PRIVATE|MAP_ANONYMOUS
    register long a4 asm("a4") = -1;
    register long a5 asm("a5") = 0;
    register long a7 asm("a7") = 222;  // SYS_mmap2
    asm volatile("ecall" : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a7)
                 : "memory");
    return (long)a0 < 0 ? 0 : (void *)a0;
}

// SIGSEGV trap: report faults (usually in the loaded program) with
// image-relative offsets instead of a bare "Segmentation fault". Installed
// via the Linux rt_sigaction syscall — picolibc's signal() is software-only
// and never reaches qemu. The pc offset 160 is where riscv32 ucontext keeps
// mcontext gregs[0] (verified under qemu 8.2).
static char *image_base;
static unsigned image_span;

static void segv_handler(int sig, void *info, void *uc)
{
    unsigned addr = ((unsigned *)info)[3];     // si_addr
    unsigned pc   = ((unsigned *)uc)[160 / 4]; // mcontext pc
    unsigned ra   = ((unsigned *)uc)[164 / 4]; // mcontext ra
    fprintf(stderr, "elfrun: SIGSEGV addr=%#x pc=%#x ra=%#x", addr, pc, ra);
    if (pc - (unsigned)image_base < image_span)
        fprintf(stderr, " (image+%#x)", pc - (unsigned)image_base);
    fprintf(stderr, "\n");
    (void)sig;
    _exit(139);
}

static void trap_segv(void)
{
    static struct { void *handler; unsigned long flags; unsigned long mask[2]; } sa;
    sa.handler = (void *)segv_handler;
    sa.flags = 4; // SA_SIGINFO
    register long a0 asm("a0") = 11;        // SIGSEGV
    register long a1 asm("a1") = (long)&sa;
    register long a2 asm("a2") = 0;
    register long a3 asm("a3") = 8;         // sigsetsize
    register long a7 asm("a7") = 134;       // SYS_rt_sigaction
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a7) : "memory");
}

// --- Minimal ELF32 definitions (picolibc has no elf.h) ---

typedef unsigned int   u32;
typedef unsigned short u16;

typedef struct {
    unsigned char e_ident[16];
    u16 e_type, e_machine;
    u32 e_version, e_entry, e_phoff, e_shoff, e_flags;
    u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Ehdr;

typedef struct {
    u32 sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size,
        sh_link, sh_info, sh_addralign, sh_entsize;
} Shdr;

typedef struct {
    u32 p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_flags, p_align;
} Phdr;

typedef struct {
    u32 st_name, st_value, st_size;
    unsigned char st_info, st_other;
    u16 st_shndx;
} Sym;

typedef struct { u32 r_offset, r_info; int r_addend; } Rela;

enum {
    ET_DYN = 3, EM_RISCV = 243, PT_LOAD = 1,
    SHT_RELA = 4, SHT_NOBITS = 8, SHF_ALLOC = 2, SHN_UNDEF = 0,
    R_RISCV_32 = 1, R_RISCV_RELATIVE = 3, R_RISCV_JUMP_SLOT = 5
};

// --- Export table: what the "firmware" offers to loaded programs ---

static const struct { const char *name; void *addr; } exports[] = {
    { "printf",  (void *)printf  }, { "puts",    (void *)puts    },
    { "putchar", (void *)putchar }, { "getchar", (void *)getchar },
    { "exit",    (void *)exit    }, { "abort",   (void *)abort   },
    { "malloc",  (void *)malloc  }, { "calloc",  (void *)calloc  },
    { "realloc", (void *)realloc }, { "free",    (void *)free    },
    { "memcpy",  (void *)memcpy  }, { "memmove", (void *)memmove },
    { "memset",  (void *)memset  }, { "memcmp",  (void *)memcmp  },
    { "strcmp",  (void *)strcmp  }, { "strncmp", (void *)strncmp },
    { "strcpy",  (void *)strcpy  }, { "strlen",  (void *)strlen  },
    { "strtol",  (void *)strtol  },
    { "open",    (void *)open    }, { "close",   (void *)close   },
    { "read",    (void *)read    }, { "write",   (void *)write   },
    { "lseek",   (void *)lseek   },
};

static void *resolve(const char *name)
{
    unsigned i;
    for (i = 0; i < sizeof(exports) / sizeof(exports[0]); i++)
        if (!strcmp(exports[i].name, name)) return exports[i].addr;
    fprintf(stderr, "elfrun: unresolved symbol: %s\n", name);
    exit(127);
}

static void die(const char *msg, const char *arg)
{
    fprintf(stderr, "elfrun: %s%s\n", msg, arg ? arg : "");
    exit(125);
}

// --- Bounded, instrumented stack (-s<bytes>) ---
//
// On device the compiler runs on a small task stack (tens of KB); under
// qemu-user it gets ~8 MB, so a stack-hungry build that overflows on an
// esp32p4 still passes. Passing -s<bytes> runs the loaded program on a painted
// region of that size and reports the high-water mark. Deliberately minimal:
// no guard page or signal plumbing (that would change behavior for normal
// runs too) -- an overflow just clobbers the bottom sentinel, which we flag.
#define STACK_PAINT 0xAA
static char stackbuf[256 * 1024]; // backs -s; bounded region is its top slice

// Switch sp to newsp, call fn(argc, argv), restore sp, return fn's value.
// s0/s1 are callee-saved, so fn preserves them across the call.
__attribute__((naked, used))
static int run_on_stack(int argc, char **argv, void *fn, void *newsp)
{
    (void)argc; (void)argv; (void)fn; (void)newsp;
    asm volatile(
        "addi sp, sp, -16\n"
        "sw   ra, 12(sp)\n"
        "sw   s0, 8(sp)\n"
        "sw   s1, 4(sp)\n"
        "mv   s0, sp\n"     // save caller stack
        "mv   s1, a2\n"     // fn
        "mv   sp, a3\n"     // switch to bounded stack
        "jalr s1\n"         // fn(a0=argc, a1=argv) -> a0
        "mv   sp, s0\n"     // restore caller stack
        "lw   ra, 12(sp)\n"
        "lw   s0, 8(sp)\n"
        "lw   s1, 4(sp)\n"
        "addi sp, sp, 16\n"
        "ret\n");
}

int main(int argc, char **argv)
{
    trap_segv();

    // Leading -s<bytes> runs the program on a bounded instrumented stack (see
    // run_on_stack). An env var would be cleaner, but semihosted picolibc
    // under qemu-user gives the guest no environment, so getenv() is useless.
    long limit = 0;
    while (argc >= 2 && argv[1][0] == '-' && argv[1][1] == 's') {
        limit = strtol(argv[1] + 2, 0, 0);
        argv++; argc--;
    }

    if (argc < 2) die("usage: elfrun [-s<bytes>] prog.elf [args...]", 0);

    FILE *f = fopen(argv[1], "rb");
    if (!f) die("cannot open ", argv[1]);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *file = malloc(fsize);
    if (!file || fread(file, 1, fsize, f) != (size_t)fsize) die("cannot read ", argv[1]);
    fclose(f);

    Ehdr *eh = (Ehdr *)file;
    if (memcmp(eh->e_ident, "\x7f""ELF\x01\x01", 6)) die("not a 32-bit LE ELF: ", argv[1]);
    if (eh->e_machine != EM_RISCV) die("not a RISC-V ELF: ", argv[1]);
    Shdr *sh = (Shdr *)(file + eh->e_shoff);
    Phdr *ph = (Phdr *)(file + eh->e_phoff);
    int i;

    // Load the image by PT_LOAD segments, like the device RISC-V loader
    // (esp_elf_load_segment): one zeroed RWX block spanning the segment vaddr
    // range, into which each segment's filesz bytes are copied; the
    // memsz-filesz tail stays zero (.bss). The device pins the load base to
    // vaddr 0 (its svaddr is forced to 0), so vaddrs, e_entry and relocation
    // offsets are all base-relative -- the first PT_LOAD must be vaddr 0.
    if (eh->e_phnum == 0) die("no program headers (PT_LOAD required): ", argv[1]);
    u32 span = 0;
    for (i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        if (ph[i].p_memsz < ph[i].p_filesz) die("PT_LOAD memsz < filesz: ", argv[1]);
        if (ph[i].p_vaddr + ph[i].p_memsz > span) span = ph[i].p_vaddr + ph[i].p_memsz;
    }
    if (span == 0) die("no loadable PT_LOAD segment: ", argv[1]);
    char *base = mmap_rwx((span + 0xfff) & ~0xfff);
    if (!base) die("mmap failed", 0);
    image_base = base;
    image_span = span;
    for (i = 0; i < eh->e_phnum; i++)
        if (ph[i].p_type == PT_LOAD)
            memcpy(base + ph[i].p_vaddr, file + ph[i].p_offset, ph[i].p_filesz);

    // Apply relocations; resolve imports by name like elf_loader does.
    for (i = 0; i < eh->e_shnum; i++) {
        if (sh[i].sh_type != SHT_RELA) continue;
        Sym *syms = (Sym *)(file + sh[sh[i].sh_link].sh_offset);
        char *strs = file + sh[sh[sh[i].sh_link].sh_link].sh_offset;
        Rela *rel = (Rela *)(file + sh[i].sh_offset);
        int n = sh[i].sh_size / sizeof(Rela);
        int j;
        for (j = 0; j < n; j++) {
            u32 *slot = (u32 *)(base + rel[j].r_offset);
            u32 type = rel[j].r_info & 0xff;
            Sym *s = syms + (rel[j].r_info >> 8);
            u32 S = 0;
            if (rel[j].r_info >> 8) {
                if (s->st_shndx != SHN_UNDEF) S = (u32)base + s->st_value;
                else S = (u32)resolve(strs + s->st_name);
            }
            if (type == R_RISCV_32)            *slot = S + rel[j].r_addend;
            else if (type == R_RISCV_RELATIVE) *slot = (u32)base + rel[j].r_addend;
            else if (type == R_RISCV_JUMP_SLOT) *slot = S;
            else { fprintf(stderr, "elfrun: unsupported relocation type %u\n", type); exit(125); }
        }
    }

    asm volatile("fence.i" ::: "memory"); // qemu wants this after writing code

    int (*entry)(int, char **) = (int (*)(int, char **))(base + eh->e_entry);

    if (limit <= 0)
        return entry(argc - 1, argv + 1); // default: qemu's full stack

    // Run on the top `limit` bytes of stackbuf (sp must stay 16-byte aligned),
    // painted so we can read the high-water mark back afterwards.
    limit = (limit + 15) & ~15L;
    if (limit > (long)sizeof(stackbuf)) die("requested stack exceeds stackbuf", 0);
    char *stack = stackbuf + sizeof(stackbuf) - limit; // grows down from the top
    memset(stack, STACK_PAINT, limit);

    int rc = run_on_stack(argc - 1, argv + 1, (void *)entry, stackbuf + sizeof(stackbuf));

    long used = limit;
    while (used > 0 && (unsigned char)stack[limit - used] == STACK_PAINT)
        used--;
    if (used >= limit) {
        fprintf(stderr, "elfrun: stack OVERFLOW (used all %ld bytes)\n", limit);
        return 139; // bottom sentinel clobbered: fail the run, rc is untrustworthy
    }
    fprintf(stderr, "elfrun: stack used %ld of %ld bytes\n", used, limit);
    return rc;
}
