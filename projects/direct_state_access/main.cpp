#include "direct_state_access.h"
#include <iostream>

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "Direct State Access";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = false;
    options.vSync = true;
    options.glVersion = { 4, 5 };
    options.backgroundColor = glm::vec4(0.051f, 0.142f, 0.191f, 1.0f);
    options.assetRootDir = "../../media/";

    return options;
}

int main(int argc, char* argv[]) {
    Options options = getOptions(argc, argv);

    try {
        DirectStateAccess app(options);
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}