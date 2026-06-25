# rcc700.c: Self-hosting mini C compiler for esp32 / RISC-V

A port of [xcc700](https://github.com/valdanylchuk/xcc700) (my Xtensa mini C
compiler) to the RISC-V esp32 variants. Mostly just RV32 ABI/opcodes instead of Xtensa.
Tested on the esp32-p4.

See the [xcc700 README](https://github.com/valdanylchuk/xcc700) for the missing C features (tl;dr: most of them) and the "why".

This time, I also include some test tooling for any brave modders. I hear, RISC-V is the future!

I would still keep this repo minimal, with any advanced versions on forks or elsewhere.

rcc700.c may also be adaptable to other RISC-V 32-bit environments, if you want a tiny toy compiler there.
There is no shortage of serious compilers for RISC-V.

## Sample output

    [ rcc700 ] BUILD COMPLETED > OK
    > IN  : 753 Lines / 8120 Tokens
    > SYM : 69 Funcs / 102 Globals / 153 Literals / 1031 Patches
    > OUT : 32444 B .text / 1180 B .rodata / 26596 B bss / 38184 B ELF

The output is a relocatable ELF that the [ESP-IDF
elf_loader](https://components.espressif.com/components/espressif/elf_loader/)
links on load against whatever your firmware exposes: newlib libc, ESP-IDF components, your own
functions.

## How to run it

**A.** Compile with `gcc rcc700.c` and run it on your computer as a cross-compiler. Works on Mac.

**B.** Build and test the native binary with `make docker-test`

**C.** Adapt the source code and call it as a function in your firmware.

## License

This is free software under MIT License, see [LICENSE](LICENSE).
