#include "io.h"

int main() {
    int a = 0, b = 0;
    char name[50];
    char ch;

    // Test integer input
    print_func("Enter first number: ");
    scan_func("%d", &a);

    print_func("Enter second number: ");
    scan_func("%d", &b);

    // Print integers
    print_func("You entered numbers: %d and %d\n", a, b);

    // Test string input
    print_func("Enter your name: ");
    scan_func("%s", name);

    // Print string
    print_func("Hello, %s!\n", name);

    // Test char input
    print_func("Enter a single character: ");
    scan_func(" %c", &ch);   

    print_func("You entered character: %c\n", ch);

    return 0;
}
