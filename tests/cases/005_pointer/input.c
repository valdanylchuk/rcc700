// From old_examples/hellobranch.c: assignment, address-of, dereference.
// Adapted to the xcc700 subset: declarations must have an initializer.
int printf(char *fmt, ...);
int main() {
    int a = 5;
    int b = 10;
    a = b;
    printf("Assignment: a = b -> a is %d (expected: 10)\n", a);
    int *p = &b;
    int result = *p;
    printf("Pointer Dereference: p = &b; *p is %d (expected: 10)\n", result);
    *p = 20;
    printf("Pointer Write: *p = 20 -> b is now %d (expected: 20)\n", b);
    return 0;
}
