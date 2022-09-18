#include <iostream>
#include "student.h"

int main() {
    /* default constructor */ {
        std::cout << "default constructor" << std::endl;
        Student s = Student();
        s.printInfo();
    }

    /* constructor with name and age */ {
        std::cout << "\nconstructor with name and age" << std::endl;
        Student s("Ben", 20, 4.0f);
        s.printInfo();
    }

    /* copy constructor */ {
        std::cout << "\ncopy constructor" << std::endl;
        Student t("Sarah", 21, 3.8f);
        Student s = t;
        s.printInfo();
    }

    /* get info */ {
        std::cout << "\nget test" << std::endl;
        Student s("Ben", 20, 4.0f);
        std::cout << "getName: " << s.getName() << std::endl;
        std::cout << "getAge: " << s.getAge() << std::endl;
        std::cout << "getGPA: " << s.getGPA() << std::endl;
    }

    /* set info */ {
        std::cout << "\nset test" << std::endl;
        Student s("Ben", 20, 4.0f);
        s.setName("NEW NAME");
        s.setAge(100);
        s.setGPA(3.75f);
        s.printInfo();
    }

    /* polymorphism */ {
        std::cout << "\npolymorphism" << std::endl;
        Person* p = new Student("Ben", 20, 4.0f);
        p->printInfo();
    }

    return 0;
}