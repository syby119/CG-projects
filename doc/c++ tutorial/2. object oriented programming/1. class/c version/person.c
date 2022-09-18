#include <stdio.h>
#include <string.h>

#include "person.h"

void initPersonDefault(Person* p) {
    p->name[0] = '\0';
    p->age = 0;
}

void initPerson(Person* p, const char* name, int age) {
    strcpy(p->name, name);
    p->age = age;
}

void initPersonCopy(Person* p, const Person* q) {
    strcpy(p->name, q->name);
    p->age = q->age;
}

void deinitPerson(Person* p) {
    p->name[0] = '\0';
    p->age = 0;
}

void assignPerson(Person* p, const Person* q) {
    strcpy(p->name, q->name);
    p->age = q->age;
}

const char* getPersonName(const Person* p) {
    return p->name;
}


void setPersonName(Person* p, const char* name) {
    strcpy(p->name, name);
}

int getPersonAge(const Person* p) {
    return p->age;
}

void setPersonAge(Person* p, int age) {
    p->age = age;
}

void printPersonInfo(const Person* p) {
    printf("A %d year old person named %s\n", p->age, p->name);
}