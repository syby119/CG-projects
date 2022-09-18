#include <iostream>
#include "student.h"

Student::Student(): _gpa(0.0f) { }

Student::Student(const std::string& name, int age, float gpa)
    : Person(name, age), _gpa(gpa) { }

Student::Student(const Student& s) {
    _name = s._name;
    _age = s._age;
    _gpa = s._gpa;
}

Student::~Student() { }

float Student::getGPA() const {
    return _gpa;
}

void Student::setGPA(float gpa) {
    _gpa = gpa;
}

void Student::printInfo() const {
    std::cout << "A " << _age << " year old " 
        << "student named " << _name 
        << " with GPA " << _gpa << std::endl;
}