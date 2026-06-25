// From old_examples/helloglobal.c: global variables and enums.
// Adapted to the xcc700 subset: no global initializers (globals are
// zero-initialized bss only), no structs, enum-typed vars become int.
int printf(char *fmt, ...);

enum TokenKind {
    T_EOF,
    T_IDENT,
    T_NUMBER,
    T_LPAREN
};

enum Flags {
    FLAG_READ  = 1,
    FLAG_WRITE = 2,
    FLAG_EXEC  = 4,
    FLAG_ALL   = 7
};

int g_simple_val;
int g_counter;
int g_bss_var;
int g_vals[T_LPAREN]; // size given as an enum constant (3)
char g_text[8];

int main() {
    g_simple_val = 100;
    int result = g_simple_val + 5;
    printf("Test 1: Read Global -> g_simple_val + 5 is %d (expected: 105)\n", result);

    g_counter = 50;
    g_counter = g_counter + 25;
    result = g_counter;
    printf("Test 2: Modify Global -> g_counter is now %d (expected: 75)\n", result);

    g_bss_var = g_bss_var + 42;
    result = g_bss_var;
    printf("Test 3: Uninitialized Global -> g_bss_var + 42 is %d (expected: 42)\n", result);

    int my_token = T_IDENT;
    result = my_token + T_NUMBER;
    printf("Test 4: Basic Enum -> T_IDENT + T_NUMBER is %d (expected: 3)\n", result);

    result = FLAG_READ + FLAG_WRITE + FLAG_EXEC;
    printf("Test 5: Explicit Enum -> FLAG_READ + WRITE + EXEC is %d (expected: 7)\n", result);

    g_vals[0] = 20;
    g_vals[1] = 30;
    result = g_vals[0] + g_vals[1];
    printf("Test 6: Global Int Array -> g_vals[0] + g_vals[1] is %d (expected: 50)\n", result);

    g_text[0] = 'H';
    g_text[1] = 'i';
    g_text[2] = '\0';
    printf("Test 7: Global Char Array -> g_text is %s (expected: Hi)\n", g_text);
    return 0;
}
