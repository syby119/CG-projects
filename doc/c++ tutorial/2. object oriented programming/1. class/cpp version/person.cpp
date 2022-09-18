#include <iostream>

#include "person.h"

Person::Person(): _name("unknown"), _age(0) { }

Person::Person(const std::string& name, int age): _name(name), _age(age) { }

Person::Person(const Person& p): _name(p._name), _age(p._age) { }

Person::~Person() { }

Person& Person::operator=(const Person& p) {
    _name = p._name;
    _age = p._age;

    return * this;
}

std::string Person::getName() const {
    return _name;
}

void Person::setName(const std::string& name) {
    _name = name;
}

int Person::getAge() const {
    return _age;
}

void Person::setAge(int age) {
    _age = age;
}

void Person::printInfo() {
    std::cout << "A " << _age << " year old " << "person named " << _name << std::endl;
}