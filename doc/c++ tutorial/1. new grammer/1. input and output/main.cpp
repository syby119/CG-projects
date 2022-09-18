#include <iostream>
#include <string>

int main() {
    std::string name;
    unsigned int number;

    std::cout << "Input name: ";
    std::cin >> name;
    std::cout << "Input school number: ";
    std::cin >> number;
    std::cout << "Greetings from " << name << "(" << number << ")." << std::endl;

    return 0;
}