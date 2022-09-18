#include <stdio.h>
#include <stdlib.h>

#define N 10

typedef struct Student {
    char name[80];
    int age;
} Student;

void example1() {
    printf("Example1: dynamic object\n");
    int* i = (int*)malloc(sizeof(int));

    *i = 100;
    printf("i = %d\n\n", *i);

    free(i);
}

void example2() {
    printf("Example2: dynamic array\n");
    Student* students = (Student*)malloc(sizeof(Student) * N);

    for (int i = 0; i < N; ++i) {
        students[i].name[0] = (char)('A' + i);
        students[i].name[1] = '\0';
        students[i].age = 20 + i;
    }

    for (int i = 0; i < N; ++i) {
        printf("students[%d] = { %s, %d }\n", i, students[i].name, students[i].age);
    }

    free(students);
}

int main() {
    example1();
    example2();

    return 0;
}