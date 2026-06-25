# rcc700 - build and test
#
# Everything here is RV32, built with the riscv32-esp-elf toolchain (the one
# ESP-IDF uses) and run under qemu-riscv32. rcc700 itself, the binaries it
# produces, and the gcc reference binaries are all ESP-IDF elf_loader-style
# shared ELFs (-nostartfiles -nostdlib -fPIC -shared); tools/elfrun.c is the
# host stand-in for the on-device elf_loader and runs them in CI. This
# mirrors the final target (esp32-c5); there is deliberately no native build.
#
# Inside Linux (or the dev container):   make test / make smoke / make ref
# On macOS (or anywhere with Docker):    make docker-test
#
# `make docker-test` is the canonical entry point: it is exactly what CI runs.

QEMU  ?= qemu-riscv32
IMAGE ?= rcc700-dev

# MCU-level stack ceiling for the self-host compile. On device the compiler
# runs on a small task stack (tens of KB); qemu-user would give it ~8 MB,
# hiding a stack-hungry build that overflows on an esp32p4. STACK=<bytes> makes
# elfrun run the self-compile on a bounded, painted stack (see tools/elfrun.c)
# and report the high-water mark. The golden suite still runs unbounded, so the
# normal scenario stays covered. Override `make test STACK=` to disable.
STACK     ?= 32768
STACKFLAG := $(if $(STACK),-s$(STACK))

RV32       := -march=rv32imac_zicsr_zifencei -mabi=ilp32
CC_RV32    ?= riscv32-esp-elf-gcc
STRIP_RV32 ?= riscv32-esp-elf-strip

# elf_loader-style shared ELF: no crt0, no libc bundled; every libc call
# stays an undefined symbol for the loader to resolve. Entry is app_main
# (-Dmain renames it), same convention as on-device.
ELF_CFLAGS := $(RV32) -O0 -fno-builtin -Dmain=app_main \
              -nostartfiles -nostdlib -fPIC -shared -fvisibility=hidden \
              -Wl,-e,app_main -Wl,--gc-sections

# elfrun is a normal static picolibc binary; qemu-user services its
# semihosting calls, so the toolchain's own crt0+libc do all the I/O.
HOST_CFLAGS := $(RV32) -O1 --specs=semihost.specs -Wall -Wextra

ELFRUN := build/elfrun
BIN    := build/rcc700.elf
SELF   := build/rcc700_self.elf
SELF2  := build/rcc700_self2.elf

.PHONY: all test smoke ref selfhost clean docker-image docker-test docker-ref docker-shell

all: $(BIN) $(ELFRUN)

$(ELFRUN): tools/elfrun.c
	@mkdir -p build
	$(CC_RV32) $(HOST_CFLAGS) -o $@ tools/elfrun.c

$(BIN): src/rcc700.c
	@mkdir -p build
	$(CC_RV32) $(ELF_CFLAGS) -Wall -Wextra -o $@ src/rcc700.c
	$(STRIP_RV32) --strip-all $@

# Verify the toolchain itself works (riscv32-esp-elf-gcc + qemu present).
smoke: $(ELFRUN)
	@command -v $(QEMU) >/dev/null || { \
	    echo "smoke: $(QEMU) not found (apt-get install qemu-user, or use make docker-test)"; exit 1; }
	@$(CC_RV32) --version | sed -n 1p
	@$(QEMU) --version | sed -n 1p
	@echo "smoke: OK"

test: $(BIN) $(ELFRUN)
	ELFRUN=$(ELFRUN) ./tests/run_tests.sh $(BIN)
	$(MAKE) selfhost

# The self-hosting test, on src/rcc700.c in place: rcc700 compiles its own
# source (stage2), stage2 compiles it again (stage3). stage2 and stage3 must
# be byte-identical, and stage2 must pass the whole golden suite.
selfhost: $(BIN) $(ELFRUN)
	# The two self-compiles run under the MCU-level stack ceiling ($(STACKFLAG));
	# elfrun prints the high-water mark and fails if it overflows. This is the
	# heaviest realistic workload, so it's where the stack check matters.
	$(QEMU) $(ELFRUN) $(STACKFLAG) $(BIN) src/rcc700.c -o $(SELF)
	$(QEMU) $(ELFRUN) $(STACKFLAG) $(SELF) src/rcc700.c -o $(SELF2)
	cmp $(SELF) $(SELF2)
	@echo "selfhost: stage2 == stage3 (fixpoint OK)"
	ELFRUN=$(ELFRUN) ./tests/run_tests.sh $(SELF)
	# No-"-o" run (argc==2): exercises the default-output path so the compiler
	# never reads argv[2] out of bounds. The golden suite always passes -o, so
	# without this the on-device `rself foo.c` invocation went untested.
	@cd build && $(QEMU) ../$(ELFRUN) ../$(SELF) ../tests/cases/001_hello/input.c >/dev/null
	@echo "selfhost: no-\"-o\" invocation OK (default a.out)"

# Build reference ELFs from the test cases with gcc (same elf_loader-style
# flags), run them via elfrun, and validate output/exit-code against the
# golden files. These are the ground-truth binaries: same behavior rcc700's
# output must match. Disassemble with: riscv32-esp-elf-objdump -d tests/ref/NNN.elf
ref: $(ELFRUN)
	@mkdir -p tests/ref
	@pass=0; fail=0; \
	for dir in tests/cases/*/; do \
	    name=$$(basename $$dir); elf=tests/ref/$$name.elf; log=tests/ref/$$name.log; \
	    $(CC_RV32) $(ELF_CFLAGS) -o $$elf $$dir/input.c 2>$$log \
	    || { echo "ref FAIL $$name (compile):"; sed 's/^/     | /' $$log; \
	         fail=$$((fail+1)); continue; }; \
	    $(QEMU) $(ELFRUN) $$elf >tests/ref/$$name.out 2>&1; rc=$$?; \
	    want_rc=0; [ -f $$dir/expected_exit ] && want_rc=$$(cat $$dir/expected_exit); \
	    want_out=$$dir/expected.txt; [ -f "$$want_out" ] || want_out=/dev/null; \
	    diff -u $$want_out tests/ref/$$name.out >tests/ref/$$name.diff 2>&1; \
	    if [ $$rc -eq $$want_rc ] && diff -q $$want_out tests/ref/$$name.out >/dev/null 2>&1; then \
	        echo "ref PASS $$name"; pass=$$((pass+1)); \
	    else \
	        echo "ref FAIL $$name (exit $$rc want $$want_rc):"; \
	        sed 's/^/     | /' tests/ref/$$name.diff; \
	        fail=$$((fail+1)); \
	    fi; \
	done; \
	echo "----"; echo "ref: pass=$$pass fail=$$fail"; [ $$fail -eq 0 ]
	$(MAKE) selfhost

clean:
	rm -rf build tests/out tests/ref

# --- Docker wrappers (use these on macOS; identical to CI) ---

docker-image:
	docker build -t $(IMAGE) .

docker-test: docker-image
	docker run --rm -v "$(CURDIR)":/work $(IMAGE) make -B test

docker-ref: docker-image
	docker run --rm -v "$(CURDIR)":/work $(IMAGE) make ref

docker-shell: docker-image
	docker run --rm -it -v "$(CURDIR)":/work $(IMAGE) bash
