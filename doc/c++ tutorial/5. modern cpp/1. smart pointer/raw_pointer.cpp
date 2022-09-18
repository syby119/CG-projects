#include <iostream>

struct Student {
    int chinese;
    int math;
    int english;

    Student(int c, int m, int e): chinese(c), math(m), english(e) { }
    ~Student() { std::cout << "~Student()" << std::endl; }
};

bool check() {
    const Student* s = new Student { 90, 100, 80 };
    if (s->chinese >= 85) {
        delete s;
        return false;
    } else if (s->math >= 90) {
        delete s;
        return false;
    } else if (s->english == 90) {
        delete s;
        return false;
    } else {
        delete s;
        return true;
    }
}

int main() {
   if (check()) {
       std::cout << "Pass" << std::endl;
   } else {
       std::cout << "Fail" << std::endl;
   }

    return 0;
}