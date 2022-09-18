#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/**
 * @brief get the file content with the given path
 * @param filepath full path of the file
 * @return file content in string for each line
 * @exception std::runtime_error
 */
std::vector<std::string> readFile(const std::string filepath) {
    std::vector<std::string> lines;
    std::ifstream fs(filepath, std::ios::in);

    try {
        fs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        std::string line;
        while (getline(fs, line)) {
            lines.push_back(line);
        }
        return lines;
    } catch (std::ifstream::failure& e) {
        if (!fs.eof()) { // getline will cause failbit at the end of the file
            throw std::runtime_error("read " + filepath + " error: " + e.what());
        }
    } catch (std::bad_alloc& e) {
        throw std::runtime_error(std::string("out of memory: ") + e.what());
    }

    return lines;
}

int main() {
    try {
        auto lines = readFile(__FILE__);
        for (const auto& line : lines) {
            std::cout << line << std::endl;
        }
    } catch(std::runtime_error& e) { // simply handle all runtime exception
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (...) { // handle all other exceptions
        std::cerr << "unknown exception" << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
} 