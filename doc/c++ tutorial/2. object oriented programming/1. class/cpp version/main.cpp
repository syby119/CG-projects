#include <iostream>
#include "person.h"

int main() {
    /* default constructor */ {
        std::cout << "default constructor" << std::endl;
        Person p = Person();
        p.printInfo();
    }

    /* constructor with name and age */ {
        std::cout << "\nconstructor with name and age" << std::endl;
        Person p("Ben", 27);
        p.printInfo();
    }

    /* copy constructor */ {
        std::cout << "\ncopy constructor" << std::endl;
        Person q("Sarah", 23);
        Person p = q;
        p.printInfo();
    }

    /* assign operator */ {
        std::cout << "\nassign operator" << std::endl;
        Person p("Ben", 27);
        Person q("Sarah", 23);

        p = q;

        p.printInfo();
    }

    /* get info */ {
        std::cout << "\nget test" << std::endl;
        Person p("Ben", 27);
        std::cout << "getName: " << p.getName() << std::endl;
        std::cout << "getAge: " << p.getAge() << std::endl;
    }

    /* set info */ {
        std::cout << "\nset test" << std::endl;
        Person p("Ben", 27);
        p.setName("NEW NAME");
        p.setAge(100);
        p.printInfo();
    }

    return 0;
}