#include <iostream>

template <typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

template <>
char max<char>(char a, char b) {
    return 'A';
}

int main() {
    std::cout << "max(1, 2): " << max(1, 2) << std::endl;
    std::cout << "max(1.0f, 2.0f): " << max(1.0f, 2.0f) << std::endl;
    std::cout << "max(1.0, 2.0): " << max(1.0, 2.0) << std::endl;

    std::cout << "max(B, C): " << max('B', 'C') << std::endl;

    return 0;
}