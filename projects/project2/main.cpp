#include "transformation.h"
#include <cstdio>
#include <iostream>

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "Transformation";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = false;
    options.vSync = true;
    options.msaa = true;
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    options.assetRootDir = "../../media/";

    return options;
}

int main(int argc, char* argv[]) {
    Options options = getOptions(argc, argv);

    try {
        Transformation app(options);
        app.run();
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Unknown Error" << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}