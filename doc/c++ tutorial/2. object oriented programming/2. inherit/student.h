#pragma once

#include "person.h"

class Student: public Person {
public:
    Student();

    Student(const std::string& name, int age, float gpa);
    
    Student(const Student& s);
    
    virtual ~Student();

    float getGPA() const;

    void setGPA(float gpa);

    virtual void printInfo() const;

private:
    float _gpa;
};