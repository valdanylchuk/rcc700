// From old_examples/hellobranch.c: if/else, while, comparisons.
int printf(char *fmt, ...);
int main() {
    int a = 5;
    int b = 10;
    int result = 0;
    if (b > a) {
        result = 1;
    }
    printf("If (true): result is %d (expected: 1)\n", result);
    result = 0;
    if (a > b) {
        result = 1;
    }
    printf("If (false): result is %d (expected: 0)\n", result);
    if (a == b) {
        result = 1;
    } else {
        result = 2;
    }
    printf("If/else: result is %d (expected: 2)\n", result);
    a = 3;
    result = 0;
    while (a > 0) {
        result = result + 10;
        a = a - 1;
    }
    printf("While loop: result is %d (expected: 30)\n", result);
    result = 0;
    if (a < b) result = result + 1;
    if (a <= 0) result = result + 10;
    if (b >= 10) result = result + 100;
    if (a != b) result = result + 1000;
    printf("Comparisons: result is %d (expected: 1111)\n", result);
    return 0;
}
