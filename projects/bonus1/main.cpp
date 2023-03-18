#include "transparency.h"
#include <cstdlib>
#include <iostream>

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "Transparency";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = false;
    options.vSync = true;
    options.msaa = false;
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.051f, 0.142f, 0.191f, 1.0f);
    options.assetRootDir = "../../media/";

    return options;
}

int main(int argc, char* argv[]) {
    Options options = getOptions(argc, argv);

    try {
        Transparency app(options);
        app.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown Error" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}