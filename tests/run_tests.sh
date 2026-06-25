#!/bin/sh
# Golden-file regression tests for rcc700.
#
# Each test lives in tests/cases/NNN_name/ and contains:
#   input.c        - program to compile
#   expected.txt   - expected stdout (optional; default: empty)
#   expected_exit  - expected exit code of the compiled program (optional; default: 0)
#   compile_fail   - if present, the *compiler* must fail; file holds expected exit code
#
# A test is reported SKIP (not FAIL) while the compiler exits 42 ("not implemented"),
# so the skeleton's CI is green from day one. Remove that behavior when codegen lands.
#
# The compiler and the binaries it makes are elf_loader-style shared ELFs;
# both run under qemu via elfrun (the host stand-in for ESP-IDF elf_loader).
#
# Usage: ELFRUN=build/elfrun tests/run_tests.sh path/to/rcc700.elf

set -u
RCC=${1:?usage: run_tests.sh path/to/rcc700.elf}
QEMU=${QEMU:-qemu-riscv32}
ELFRUN=${ELFRUN:-build/elfrun}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/tests/out"
mkdir -p "$OUT"

pass=0; fail=0; skip=0

for dir in "$ROOT"/tests/cases/*/; do
    name=$(basename "$dir")
    elf="$OUT/$name.elf"
    log="$OUT/$name.stdout"

    "$QEMU" "$ELFRUN" "$RCC" "$dir/input.c" -o "$elf" > "$OUT/$name.compile" 2>&1
    cc_rc=$?

    if [ "$cc_rc" -eq 42 ]; then
        echo "SKIP $name (compiler not implemented)"
        skip=$((skip+1)); continue
    fi

    if [ -f "$dir/compile_fail" ]; then
        want_rc=$(cat "$dir/compile_fail")
        if [ "$cc_rc" -eq "$want_rc" ]; then
            echo "PASS $name (compile failed as expected)"
            pass=$((pass+1))
        else
            echo "FAIL $name: compiler exit $cc_rc, expected $want_rc"
            fail=$((fail+1))
        fi
        continue
    fi

    if [ "$cc_rc" -ne 0 ]; then
        echo "FAIL $name: compiler exit $cc_rc"
        sed 's/^/     | /' "$OUT/$name.compile"
        fail=$((fail+1)); continue
    fi

    "$QEMU" "$ELFRUN" "$elf" > "$log" 2>&1
    run_rc=$?

    want_rc=0
    [ -f "$dir/expected_exit" ] && want_rc=$(cat "$dir/expected_exit")

    want_out="$dir/expected.txt"
    [ -f "$want_out" ] || want_out=/dev/null

    ok=1
    [ "$run_rc" -eq "$want_rc" ] || ok=0
    diff -u "$want_out" "$log" > "$OUT/$name.diff" 2>&1 || ok=0

    if [ "$ok" -eq 1 ]; then
        echo "PASS $name"
        pass=$((pass+1))
    else
        echo "FAIL $name: exit $run_rc (want $want_rc)"
        sed 's/^/     | /' "$OUT/$name.diff"
        fail=$((fail+1))
    fi
done

echo "----"
echo "pass=$pass fail=$fail skip=$skip"
[ "$fail" -eq 0 ]
