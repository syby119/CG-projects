#include <iostream>
#include <string>
#include <map>

void print(std::map<std::string, int>& m) {
    for (auto it = m.begin(); it != m.end(); ++it) {
        std::cout << it->first << " -> " << it->second << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Map definition" << std::endl;
    std::map<std::string, int> m;
    std::cout << "Map size: " << m.size() << std::endl;

    // add new elements
    std::cout << "\nAdd new elements" << std::endl;
    m["one"] = 1;
    m["two"] = 2;
    m["three"] = 3;
    print(m);

    // tranlerse the map by iterator
    std::cout << "\nTranverse the map" << std::endl;
    print(m);

    // modify map
    std::cout << "\nMap modify" << std::endl;
    m["one"] = 100;
    print(m);

    std::cout << "\nMap deletion" << std::endl;
    m.erase("one");
    print(m);

    return 0;
}