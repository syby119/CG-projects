#include <iostream>
#include <string>

constexpr int N = 10;

struct Student {
    std::string name;
    int age;
};

void example1() {
    printf("Example1: dynamic object\n");
    int* i = new int;

    *i = 100;
    std::cout << "i = " << *i << '\n' << std::endl;

    delete i;
}

void example2() {
    printf("Example2: dynamic array\n");
    Student* students = new Student[N];

    for (int i = 0; i < N; ++i) {
        students[i].name = std::string(1, (char)('A' + i));
        students[i].age = 20 + i;
    }

    for (int i = 0; i < N; ++i) {
        std::cout << "students[" << i <<  "] = { " 
                  << students[i].name << ", " << students[i].age << " }" << std::endl;
    }

    delete[] students;
}

int main() {
    example1();
    example2();

    return 0;
}