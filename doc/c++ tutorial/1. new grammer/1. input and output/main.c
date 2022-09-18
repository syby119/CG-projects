#include <stdio.h>

int main() {
    char name[80];
    unsigned int number;

    printf("Input name: ");
    scanf("%s", name);
    printf("Input school number: ");
    scanf("%u", &number);

    printf("Greetings from %s(%u).\n", name, number);

    return 0;
}