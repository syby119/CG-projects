#pragma once 

#include <string>

class Person {
public:
    // default constructor
    Person();

    // constructor with parameters
    Person(const std::string& name, int age);
    
    // copy constructor
    Person(const Person& p);
    
    // destructor
    ~Person();

    // assign operator
    Person& operator=(const Person& p);

    // get name method
    std::string getName() const;

    // set name method
    void setName(const std::string& name);

    // get age method
    int getAge() const;
    
    // set age method
    void setAge(int age);

    // print info
    void printInfo();

private:
    std::string _name;
    int _age;
};
