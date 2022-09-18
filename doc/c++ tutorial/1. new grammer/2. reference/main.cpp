#include <iostream>

void update(int& ref, int value) {
    ref = value;
}

void example1() {
    int i = 0;
    int& iRef = i;

    std::cout << "Example1: refence as aliasing" << std::endl;

    std::cout << "1. Initial state" << std::endl;
    std::cout << "i: " << i << std::endl;
    std::cout << "iRef: " << iRef << '\n' << std::endl;

    std::cout << "2. Change i value" << std::endl;
    i = 100;
    std::cout << "i: " << i << std::endl;
    std::cout << "iRef: " << iRef << '\n' << std::endl;

    std::cout << "3. Change iRef value" << std::endl;
    iRef = 200;
    std::cout << "i: " << i << std::endl;
    std::cout << "iRef: " << iRef << '\n' << std::endl;
}

void example2() {
    int i = 0;
    int& iRef = i;
    
    std::cout << "Example2: refence as function parameters" << std::endl;
    std::cout << "1. Initial state" << std::endl;
    std::cout << "i: " << i << std::endl;
    std::cout << "iRef: " << iRef << '\n' << std::endl;

    std::cout << "2. Pass i to update" << std::endl;
    update(i, 100);
    std::cout << "i: " << i << std::endl;
    std::cout << "iRef: " << iRef << '\n' << std::endl;

    std::cout << "3. Pass iRef to update" << std::endl;
    update(iRef, 200);
    std::cout << "i: " << i << std::endl;
    std::cout << "iRef: " << iRef << '\n' << std::endl;
}

int main() {
    example1();
    example2();

    return 0;
}