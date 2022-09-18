#include <iostream>

struct A {
    int i = 100;
    int j = 200;
    int k = 300;

    A() {}
    A(int j): j(j) {}
    void print() {
        std::cout << "{ " << i << ", " << j << ", " << k << " }" << std::endl;
    }
};

void case1() {
    std::cout << "Default case" << std::endl;
    A a;
    a.print();
}

void case2() {
    std::cout << "\nConflicting case" << std::endl;
    A a(500);
    a.print();
}

int main() {
    case1();
    case2();

    return 0;
}
