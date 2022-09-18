#pragma once 

#define MAX_NAME_LEN 80

typedef struct Person {
    char name[MAX_NAME_LEN];
    int age;
} Person;

void initPersonDefault(Person* p);
void initPerson(Person* p, const char* name, int age);
void initPersonCopy(Person* p, const Person* q);
void deinitPerson(Person* p);
void assignPerson(Person* p, const Person* q);
const char* getPersonName(const Person* p);
void setPersonName(Person* p, const char* name);
int getPersonAge(const Person* p);
void setPersonAge(Person* p, int age);
void printPersonInfo(const Person* p);