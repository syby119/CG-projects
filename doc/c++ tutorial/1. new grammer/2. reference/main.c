#include <stdio.h>

void update(int* p, int value) {
    *p = value;
}

void example1() {
    int i = 0;
    int* iPtr = &i;

    printf("Example1: refence as aliasing\n");

    printf("1. Initial state\n");
    printf("i: %d\n", i);
    printf("*iPtr: %d\n\n", *iPtr);

    printf("2. Change i value\n");
    i = 100;
    printf("i: %d\n", i);
    printf("*iPtr: %d\n\n", *iPtr);

    printf("3. Change the value iPtr pointing to\n");
    *iPtr = 200;
    printf("i: %d\n", i);
    printf("*iPtr: %d\n\n", *iPtr);
}

void example2() {
    int i = 0;
    int* iPtr = &i;
    
    printf("Example2: refence as function parameters\n");
    printf("1. Initial state\n");
    printf("i: %d\n", i);
    printf("*iPtr: %d\n\n", *iPtr);

    printf("2. Pass the address of i to update\n");
    update(&i, 100);
    printf("i: %d\n", i);
    printf("*iPtr: %d\n\n", *iPtr);

    printf("3. Pass iPtr to update\n");
    update(iPtr, 200);
    printf("i: %d\n", i);
    printf("*iPtr: %d\n\n", *iPtr);
}

int main() {
    example1();
    example2();

    return 0;
}