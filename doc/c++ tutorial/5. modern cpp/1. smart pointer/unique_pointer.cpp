#include <iostream>
#include <memory>

struct Student {
    int chinese;
    int math;
    int english;

    Student(int c, int m, int e): chinese(c), math(m), english(e) { }
    ~Student() { std::cout << "~Student()" << std::endl; }
};

bool check() {
    std::unique_ptr<Student> s(new Student { 90, 100, 80 });
    if (s->chinese >= 85) {
        return false;
    } else if (s->math >= 90) {
        return false;
    } else if (s->english == 90) {
        return false;
    } else {
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