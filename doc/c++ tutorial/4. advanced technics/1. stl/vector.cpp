#include <iostream>
#include <algorithm>
#include <vector>

void print(std::vector<int>& v) {
    for (auto it = v.begin(); it != v.end(); ++it) {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Vector definition" << std::endl;
    std::vector<int> v;
    std::cout << "Vector size: " << v.size() << std::endl;

    // add new elements
    std::cout << "\nAdd new elements" << std::endl;
    for (int i = 0; i < 10; ++i) {
        v.push_back(i);
    }
    print(v);

    // tranverse the vector by iterator
    std::cout << "\nTranverse the vector" << std::endl;
    print(v);

    // modify vector
    std::cout << "\nVector insertion" << std::endl;
    v.insert(v.begin(), 100);
    print(v);


    std::cout << "\nVector deletion" << std::endl;
    v.erase(v.end() - 1);
    print(v);


    // sort the vector
    std::cout << "\nSort the vector" << std::endl;
    sort(v.begin(), v.end());
    print(v);


    return 0;
}