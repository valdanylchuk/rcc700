// Regression: parameters with type qualifiers / multi-keyword types.
// rcc700 has no `const` keyword, so `const` lexes as a plain identifier; the
// parameter parser must skip a run of leading type-specifier/qualifier tokens
// and take the last identifier as the name. Before the fix, `const char *s`
// aborted the compiler with "not implemented: parameter (token 257)" — token
// 257 being `char`, hit where the parser only expected one leading type token.
// (const is exercised only in parameter lists; rcc700 has no const elsewhere.)
int printf(const char *fmt, ...);

int slen(const char *s) {
    int n = 0;
    while (s[n] != 0) ++n;
    return n;
}

int sum_const(const int a, const int b) {
    return a + b;
}

int main() {
    char *msg = "hello";
    printf("len=%d\n", slen(msg));
    printf("sum=%d\n", sum_const(19, 23));
    return 0;
}
