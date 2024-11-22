#include <cstdlib>
#include <iostream>

#include "spirv_dynamic_compilation.h"

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "SPIRV dynamic compilation";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = false;
    options.vSync = true;
    options.msaa = true;
    options.glVersion = { 4, 6 };
    options.backgroundColor = glm::vec4(0.051f, 0.142f, 0.191f, 1.0f);
    options.assetRootDir = "../../media/";

    return options;
}

int main(int argc, char* argv[]) {
    Options options = getOptions(argc, argv);

    try {
        SpirvDynamicCompilation app(options);
        app.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown Error" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}