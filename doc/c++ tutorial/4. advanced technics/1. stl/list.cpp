#include <iostream>
#include <list>

void print(std::list<int>& l) {
    for (auto it = l.begin(); it != l.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "List definition" << std::endl;
    std::list<int> l;
    std::cout << "List size: " << l.size() << std::endl;

    // add new elements
    std::cout << "\nAdd new elements" << std::endl;
    for (int i = 0; i < 10; ++i) {
        l.push_back(i);
    }
    print(l);

    // tranlerse the list by iterator
    std::cout << "\nTranverse the list" << std::endl;
    print(l);

    // modify list
    std::cout << "\nList insertion" << std::endl;
    l.insert(l.begin(), 100);
    print(l);

    std::cout << "\nList deletion" << std::endl;
    std::list<int>::iterator it = l.end();
    l.erase(--it);
    print(l);

    // sort the list
    std::cout << "\nSort the list" << std::endl;
    l.sort();
    print(l);

    return 0;
}