#include <stdio.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void implementation1() {
    printf("Implement max with macro:\n");
    printf("max(%d, %d): %d\n", 1, 2, MAX(1, 2));
    printf("max(%.3f, %.3f): %.3f\n", 1.0f, 2.0f, MAX(1.0f, 2.0f));
    printf("max(%.3f, %.3f): %.3f\n", 1.0, 2.0, MAX(1.0, 2.0));
    
    printf("max(%c, %c): %c\n", 'B', 'C', MAX('B', 'C'));
}

int maxInt(int a, int b) {
    return a > b ? a : b;
}

float maxFloat(float a, float b) {
    return a > b ? a : b;
}

double maxDouble(double a, double b) {
    return a > b ? a : b;
}

char maxChar(char a, char b) {
    return 'A';
}

void implementation2() {
    printf("\nImplement max for each data type:\n");
    printf("max(%d, %d): %d\n", 1, 2, maxInt(1, 2));
    printf("max(%.3f, %.3f): %.3f\n", 1.0f, 2.0f, maxFloat(1.0f, 2.0f));
    printf("max(%.3f, %.3f): %.3f\n", 1.0, 2.0, maxDouble(1.0, 2.0));
    
    printf("max(%c, %c): %c\n", 'B', 'C', maxChar('B', 'C'));
}

int main() {
    implementation1();
    implementation2();

    return 0;
}