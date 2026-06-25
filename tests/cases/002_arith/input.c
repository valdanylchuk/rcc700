int printf(char *fmt, ...);
int main() {
    int a = 6;
    int b = 7;
    printf("%d\n", a * b);
    printf("%d\n", (a + b) % 4);
    // hellomath.c cases: precedence, parens, division, modulus
    int x = 20;
    int y = 10;
    int z = 2;
    printf("%d\n", x + y - z);       // 28
    printf("%d\n", y + z * 5);       // 20: * before +
    printf("%d\n", (y + z) * 5);     // 60: parens override
    printf("%d\n", x / y % z);       // 0: left-to-right
    printf("%d\n", x + y / (z + 3)); // 22
    return 0;
}
