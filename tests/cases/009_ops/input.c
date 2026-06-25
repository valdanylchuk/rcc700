// Operator coverage within the xcc700 feature scope, merged from
// old_examples/helloops.c and old_examples/test300.c.
// Out of scope (not in xcc700): postfix ++/--, compound assignment
// (+=, -=, ...), sizeof, structs, casts.
int printf(char *fmt, ...);
int check(int actual, int expected, char *desc);

int g_count;

int main() {
    int total = 0; int passed = 0;
    int res = 0;

    // Unary operators
    int u = 10;
    res = -u;       passed = passed + check(res, -10, "Unary Neg (-)"); ++total;
    res = -(u + 5); passed = passed + check(res, -15, "Unary Neg Expr"); ++total;
    res = ~0;       passed = passed + check(res, -1, "Unary BitNot (~0)"); ++total;
    res = ~60;      passed = passed + check(res, -61, "Unary BitNot (~60)"); ++total;
    res = !u;       passed = passed + check(res, 0, "Unary Logical NOT (!10)"); ++total;
    res = !0;       passed = passed + check(res, 1, "Unary Logical NOT (!0)"); ++total;
    res = !!u;      passed = passed + check(res, 1, "Unary Logical NOT (!!10)"); ++total;

    // Prefix increment/decrement
    int j = 10; int l = 10;
    res = ++j; passed = passed + check(res, 11, "Pre-increment returns new value"); ++total;
    passed = passed + check(j, 11, "Value after pre-increment"); ++total;
    res = --l; passed = passed + check(res, 9, "Pre-decrement returns new value"); ++total;
    passed = passed + check(l, 9, "Value after pre-decrement"); ++total;
    g_count = 5;
    res = ++g_count; passed = passed + check(res, 6, "Pre-increment on global"); ++total;

    // Bitwise operators
    int a = 60; int b = 13; // 60=00111100, 13=00001101
    res = a & b;  passed = passed + check(res, 12, "Bitwise AND (60 & 13)"); ++total;
    res = a | b;  passed = passed + check(res, 61, "Bitwise OR (60 | 13)"); ++total;
    res = a ^ b;  passed = passed + check(res, 49, "Bitwise XOR (60 ^ 13)"); ++total;
    res = a << 2; passed = passed + check(res, 240, "Shift Left (60 << 2)"); ++total;
    res = a >> 2; passed = passed + check(res, 15, "Shift Right (60 >> 2)"); ++total;
    res = -8 >> 1; passed = passed + check(res, -4, "Shift Right is arithmetic"); ++total;

    // Logical operators (like xcc700: value-normalizing, not short-circuit)
    res = 1 && 0; passed = passed + check(res, 0, "Logical AND (1&&0)"); ++total;
    res = 5 && 3; passed = passed + check(res, 1, "Logical AND normalizes (5&&3)"); ++total;
    res = 1 || 0; passed = passed + check(res, 1, "Logical OR (1||0)"); ++total;
    res = 0 || 0; passed = passed + check(res, 0, "Logical OR (0||0)"); ++total;

    // Precedence
    res = 1 | 2 ^ 3 & 2; passed = passed + check(res, 1, "Precedence (& before ^ before |)"); ++total;
    res = 1 << 2 + 1;    passed = passed + check(res, 8, "Precedence (+ before <<)"); ++total;
    res = 5 & 3 == 3;    passed = passed + check(res, 1, "Precedence (== before &)"); ++total;
    res = 1 && 0 || 1;   passed = passed + check(res, 1, "Precedence (&& before ||)"); ++total;

    // Ternary
    int t_val = 1 ? 100 : 200;
    passed = passed + check(t_val, 100, "Ternary True"); ++total;
    t_val = 0 ? 100 : 200;
    passed = passed + check(t_val, 200, "Ternary False"); ++total;
    int x = 5;
    t_val = (x > 2) ? 11 : 22;
    passed = passed + check(t_val, 11, "Ternary Expr Cond"); ++total;
    t_val = 0 ? 1 : 1 ? 2 : 3;
    passed = passed + check(t_val, 2, "Ternary Nested"); ++total;
    t_val = (x > 2) ? x + 10 : x - 10;
    passed = passed + check(t_val, 15, "Ternary Branch Exprs"); ++total;

    printf("-----------------------------\n");
    printf("Tests Passed: %d / %d\n", passed, total);
    if (passed == total) printf("RESULT: SUCCESS\n");
    else printf("RESULT: FAIL\n");
    return 0;
}

int check(int actual, int expected, char *desc) {
    if (actual == expected) { printf("[OK]   %s\n", desc); return 1; }
    printf("[FAIL] %s -> Expected: %d, Got: %d\n", desc, expected, actual); return 0;
}
