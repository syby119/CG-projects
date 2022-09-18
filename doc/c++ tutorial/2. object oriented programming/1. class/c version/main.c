#include <stdio.h>
#include "person.h"

int main() {
    /* default constructor */ {
        printf("Init person\n");
        Person p;
        initPersonDefault(&p);

        printPersonInfo(&p);
    }

    /* constructor with name and age */ {
        printf("\nInit person with name and age\n");
        Person p;
        initPerson(&p, "Ben", 27);

        printPersonInfo(&p);
    }

    /* copy constructor */ {
        printf("\nInit person with another person\n");
        Person q;
        initPerson(&q, "Sarah", 23);

        Person p;
        initPersonCopy(&p, &q);

        printPersonInfo(&p);
    }

    /* assign operator */ {
        printf("\nAssign person with another person\n");
        Person p, q;
        initPerson(&p, "Ben", 27);
        initPerson(&q, "Sarah", 23);

        assignPerson(&p, &q);

        printPersonInfo(&p);
    }

    /* get info */ {
        printf("\nGet test\n");
        Person p;
        initPerson(&p, "Ben", 27);
        printf("name: %s\n", getPersonName(&p));
        printf("age: %d\n", getPersonAge(&p));
    }

    /* set info */ {
        printf("\nSet test\n");
        Person p;
        initPerson(&p, "Ben", 27);
        setPersonName(&p, "NEW NAME");
        setPersonAge(&p, 100);
        printPersonInfo(&p);
    }

    return 0;
}