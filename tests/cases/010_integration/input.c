// From old_examples/test300.c: functions, pointers, arrays, chars, globals,
// recursion, and compiler-like pointer patterns (self-hosting prep).
// Adapted to the xcc700 subset: no static, no casts ((char)300 became a
// plain assignment; same truncation). Operator sections live in 009_ops,
// basic arithmetic in 002_arith.
int printf(char *fmt, ...);
int check(int actual, int expected, char *desc);
int add_vals(int a, int b);
int mult_vals(int m, int n);
int complex_op(int c, int d);
int gcd(int a, int b);
void swap(int *x, int *y);
void set_global(int v);
void modify_char_array(char *buf);
int recurse_deep(int depth);
int mock_lexer_next();

// --- Global Variable Declarations (bss only) ---
int g_val;
int g_arr[5];
char g_char;
char g_c_arr[4];
int *g_ptr;
char *g_str_ptr;

enum {
    VAL_ZERO,       // 0
    VAL_ONE,        // 1
    VAL_TEN = 10,   // 10
    VAL_ELEVEN      // 11
};

int main() {
    int total = 0; int passed = 0;
    int res = 0;

    printf("--- Starting tests ---\n");

    // --- Function Calls ---
    res = add_vals(10, 5);   passed = passed + check(res, 15, "Func Call"); ++total;
    res = complex_op(10, 5); passed = passed + check(res, 65, "Nested Funcs"); ++total;

    // --- Control Flow & Prefix Ops ---
    int i = 5; int sum = 0;
    while (i > 0) { sum = sum + i; --i; }
    passed = passed + check(sum, 15, "While Loop (Sum 1..5)"); ++total;

    res = gcd(48, 18);
    passed = passed + check(res, 6, "While Loop (GCD)"); ++total;

    // --- Pointers ---
    int target = 999;
    int *p = &target;
    int read_val = *p;   passed = passed + check(read_val, 999, "Ptr: Read (*p)"); ++total;
    *p = 111;            passed = passed + check(target, 111,   "Ptr: Write (*p)"); ++total;

    int **pp = &p;
    int p_val = **pp;    passed = passed + check(p_val, 111,    "Ptr: Double Deref (**pp)"); ++total;

    int s1 = 100; int s2 = 200; int swap_ok = 0;
    swap(&s1, &s2);
    if (s1 == 200) { if (s2 == 100) swap_ok = 1; }
    passed = passed + check(swap_ok, 1, "Ptr: Func Args (Swap)"); ++total;

    // --- Arrays ---
    int arr[5];
    arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40; arr[4] = 50;
    int arr_sum = arr[0] + arr[1];
    passed = passed + check(arr_sum, 30, "Array: Read/Write"); ++total;

    int idx = 2;
    int val_idx = arr[idx];
    passed = passed + check(val_idx, 30, "Array: Var Indexing"); ++total;

    // Loop fill (Fibonacci: 0, 1, 1, 2, 3, 5, 8...)
    int fib[10]; fib[0] = 0; fib[1] = 1; int k = 2;
    while (k < 10) { fib[k] = fib[k-1] + fib[k-2]; ++k; }
    passed = passed + check(fib[9], 34, "Array: Loop/Logic (Fib)"); ++total;

    // Pointer Decay & Indexing via Pointer
    int *ptr = arr;
    ptr[1] = 99; // Scales index by type size
    passed = passed + check(arr[1], 99, "Array: Write via Ptr"); ++total;
    int ptr_read = ptr[4];
    passed = passed + check(ptr_read, 50, "Array: Read via Ptr"); ++total;

    // --- Char Support ---
    char c_val = 0; char c_arr[4]; char *c_ptr = 0;

    c_val = 'A'; passed = passed + check(c_val, 'A', "Char: Assign"); ++total;

    // Truncation: 300 (0x12C) -> 44 (0x2C). Char slot holds the low byte.
    c_val = 300; passed = passed + check(c_val, 44, "Char: Truncation"); ++total;

    // Stack packing check (ensure bytes don't overlap like words)
    c_arr[0] = 10; c_arr[1] = 20; c_arr[2] = 30;
    int arr_val = c_arr[1];
    passed = passed + check(arr_val, 20, "Char Array: Indexing"); ++total;

    c_ptr = "ABC";
    int char_0 = *c_ptr;   passed = passed + check(char_0, 'A', "Char Ptr: Deref"); ++total;
    int char_1 = c_ptr[1]; passed = passed + check(char_1, 'B', "Char Ptr: Indexing"); ++total;

    int mixed_res = c_arr[0] + c_ptr[2]; // 10 + 67
    passed = passed + check(mixed_res, 77, "Char: Mixed Math"); ++total;

    // Modifying byte at offset 1 via pointer
    c_ptr = c_arr; c_ptr[1] = 99;
    passed = passed + check(c_arr[1], 99, "Char Ptr: Write Stack"); ++total;

    // Unsigned behavior: -1 (0xFF) -> 255 (lbu zero-extends)
    c_val = 0 - 1;
    passed = passed + check(c_val, 255, "Char: Unsigned (lbu)"); ++total;

    // Array passed to a function that writes through the pointer
    modify_char_array(c_arr);
    passed = passed + check(c_arr[2], 'X', "Char Ptr: Func Arg Write"); ++total;

    // --- Global Variables (bss) ---
    passed = passed + check(g_val, 0, "Global: Init (0)"); ++total;
    g_val = 12345;
    passed = passed + check(g_val, 12345, "Global: Read/Write"); ++total;
    set_global(999);
    passed = passed + check(g_val, 999, "Global: Persistence"); ++total;
    g_arr[2] = 42;
    passed = passed + check(g_arr[2], 42, "Global: Array"); ++total;
    g_char = 'X';
    passed = passed + check(g_char, 'X', "Global: Char"); ++total;
    g_c_arr[3] = 100;
    passed = passed + check(g_c_arr[3], 100, "Global: Char Array"); ++total;
    int local = 777;
    g_ptr = &local;
    int from_ptr = *g_ptr;
    passed = passed + check(from_ptr, 777, "Global: Pointer"); ++total;

    g_str_ptr = "Test";
    int g_char_val = *g_str_ptr;
    passed = passed + check(g_char_val, 'T', "Global Char Ptr: Deref"); ++total;

    g_str_ptr = g_str_ptr + 1; // Increment
    g_char_val = *g_str_ptr;
    passed = passed + check(g_char_val, 'e', "Global Char Ptr: Inc"); ++total;

    // --- Global Array Decay & Pointers ---
    g_c_arr[0] = 'H'; g_c_arr[1] = 'i'; g_c_arr[2] = 0;
    char *local_ptr = g_c_arr;
    int ch = *local_ptr;
    passed = passed + check(ch, 'H', "Global Arr Decay: Read"); ++total;
    *local_ptr = 'B';
    passed = passed + check(g_c_arr[0], 'B', "Global Arr Decay: Write"); ++total;
    char *offset_ptr = g_c_arr + 1;
    *offset_ptr = 'y';
    passed = passed + check(g_c_arr[1], 'y', "Global Arr + 1: Write"); ++total;

    // --- Enum Constants ---
    passed = passed + check(VAL_ZERO, 0, "Enum Auto (0)"); ++total;
    passed = passed + check(VAL_ONE, 1,  "Enum Auto (1)"); ++total;
    passed = passed + check(VAL_TEN, 10, "Enum Manual (10)"); ++total;
    passed = passed + check(VAL_ELEVEN, 11, "Enum Auto-After (11)"); ++total;

    // --- Deep Recursion & Pointer Patterns ---
    // Recurse on each char of a 12-char string, verify stack integrity on unwind.
    g_str_ptr = "A0A1A2A3A4A5";
    g_c_arr[0] = 0; g_c_arr[1] = 0; g_c_arr[2] = 0; g_c_arr[3] = 0;

    int recursion_depth = recurse_deep(0);
    passed = passed + check(recursion_depth, 12, "Recursion Depth (12)"); ++total;
    // recurse_deep writes 'X' to g_c_arr[2] at depth 10
    passed = passed + check(g_c_arr[2], 'X', "Recursion Side-Effect"); ++total;

    // Mock lexer: skip space, check two-char ops, parse string (next() patterns)
    g_str_ptr = "  != ++ \"AB\" ";

    res = mock_lexer_next();
    passed = passed + check(res, 2, "Ptr Logic: !="); ++total;
    res = mock_lexer_next();
    passed = passed + check(res, 1, "Ptr Logic: ++"); ++total;
    res = mock_lexer_next();
    passed = passed + check(res, 99, "Ptr Logic: String Type"); ++total;
    passed = passed + check(g_c_arr[0], 'A', "Ptr Logic: Str Content[0]"); ++total;
    passed = passed + check(g_c_arr[1], 'B', "Ptr Logic: Str Content[1]"); ++total;
    passed = passed + check(g_c_arr[2], 0,   "Ptr Logic: Str Null Term"); ++total;

    // --- Summary ---
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
int add_vals(int a, int b) { return a + b; }
int mult_vals(int m, int n) { return m * n; }
int complex_op(int c, int d) { return add_vals(c, d) + mult_vals(c, d); }
int gcd(int a, int b) {
    int t = 0;
    while (b != 0) { t = b; b = a % b; a = t; }
    return a;
}
void swap(int *x, int *y) { int tmp = *x; *x = *y; *y = tmp; }
void set_global(int v) { g_val = v; }
void modify_char_array(char *buf) { buf[2] = 'X'; }

int recurse_deep(int depth) {
    // Local variable to ensure the stack frame is preserved across the call
    int local_check = depth * 10 + 555;

    if (*g_str_ptr == 0) return 0;

    char current = *g_str_ptr;
    g_str_ptr = g_str_ptr + 1; // Advance global pointer

    // Global array access during recursion: write marker at depth 10
    if (depth == 10) g_c_arr[2] = 'X';

    int ret = recurse_deep(depth + 1);

    // Verify local variable survived (stack corruption check)
    if (local_check != depth * 10 + 555) {
        printf("FATAL: Stack corruption at depth %d\n", depth);
        return -1;
    }

    return ret + 1;
}

// A simplified version of the compiler's own next() logic.
// Uses g_str_ptr as input 'src', g_c_arr as output buffer.
// Returns: 0=EOF, 1=INC(++), 2=NE(!=), 99=STR
int mock_lexer_next() {
    while (*g_str_ptr == ' ' || *g_str_ptr == '\n') {
        g_str_ptr = g_str_ptr + 1;
    }

    if (*g_str_ptr == 0) return 0; // EOF

    // Lookahead [0] and [1], like: if (src[0]=='!' && src[1]=='=')
    if (g_str_ptr[0] == '!' && g_str_ptr[1] == '=') {
        g_str_ptr = g_str_ptr + 2;
        return 2; // T_NE
    }
    if (g_str_ptr[0] == '+' && g_str_ptr[1] == '+') {
        g_str_ptr = g_str_ptr + 2;
        return 1; // T_INC
    }

    // String parsing, like: if (*src == '"') ...
    if (*g_str_ptr == '"') {
        g_str_ptr = g_str_ptr + 1; // Skip opening quote
        char *dest = g_c_arr;
        while (*g_str_ptr != 0 && *g_str_ptr != '"') {
            *dest = *g_str_ptr;
            dest = dest + 1;
            g_str_ptr = g_str_ptr + 1;
        }
        *dest = 0; // Null terminate
        if (*g_str_ptr == '"') g_str_ptr = g_str_ptr + 1; // Skip closing quote
        return 99; // T_STR
    }

    g_str_ptr = g_str_ptr + 1;
    return -1; // Unknown
}
