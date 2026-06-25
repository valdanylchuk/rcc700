// From old_examples/hellocharr.c: char vars, char literals, arrays, indexing.
// Adapted to the xcc700 subset: no array initializers (fixed size + manual
// fill), string literals only as char* pointers.
int printf(char *fmt, ...);
int main() {
    char initial = 'A';
    printf("Simple char: initial is %c (expected: A)\n", initial);
    char next_char = initial + 3;
    printf("Char arithmetic: 'A' + 3 is %c (expected: D)\n", next_char);
    char *greeting = "Hello";
    printf("String pointer: greeting is %s (expected: Hello)\n", greeting);
    char second_char = greeting[1];
    printf("Indexing: greeting[1] is %c (expected: e)\n", second_char);
    char name[4];
    name[0] = 'J';
    name[1] = 'u';
    name[2] = 'l';
    name[3] = '\0';
    printf("Manual string: name is %s (expected: Jul)\n", name);
    name[0] = 'Y';
    printf("Modification: name is now %s (expected: Yul)\n", name);
    int ages[4];
    ages[0] = 35;
    ages[1] = 35;
    ages[2] = 12;
    ages[3] = 9;
    printf("Int array: ages[2] = %d (expected: 12)\n", ages[2]);
    return 0;
}
