// From old_examples/hellofun.c: user functions, parameters, recursion,
// nested calls. Adapted to the xcc700 subset: char *str instead of
// char str[] parameter, no array initializers (manual fill).
int printf(char *fmt, ...);
int power(int base, int exp);
void replace_char(char *str, char find, char replace);
int factorial(int n);

int main() {
    int p = power(3, 4);
    printf("Test 1: power(3, 4) -> %d (expected: 81)\n", p);

    char message[8];
    message[0] = 'g';
    message[1] = 'o';
    message[2] = ' ';
    message[3] = 'g';
    message[4] = 'o';
    message[5] = '\0';
    replace_char(message, ' ', '_');
    printf("Test 2: replace_char -> %s (expected: go_go)\n", message);

    int f = factorial(5);
    printf("Test 3: factorial(5) -> %d (expected: 120)\n", f);

    printf("Test 4: Nested call factorial(power(2,2)) -> %d (expected: 24)\n", factorial(power(2, 2)));
    return 0;
}

int power(int base, int exp) {
    int result = 1;
    while (exp > 0) {
        result = result * base;
        exp = exp - 1;
    }
    return result;
}

void replace_char(char *str, char find, char replace) {
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == find) {
            str[i] = replace;
        }
        i = i + 1;
    }
}

int factorial(int n) {
    if (n < 2) {
        return 1;
    }
    return n * factorial(n - 1);
}
