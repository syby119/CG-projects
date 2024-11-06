#include <cstdlib>
#include <iostream>

#include "mesh_shading_pipeline.h"

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "MeshShader";
    options.windowWidth = 3840;
    options.windowHeight = 2160;
    options.windowResizable = false;
    options.vSync = false;
    options.msaa = true;
    options.glVersion = {4, 5};
    options.backgroundColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    options.assetRootDir = "../../media/";

    return options;
}

int main(int argc, char* argv[]) {
    Options options = getOptions(argc, argv);

    try {
        MeshShadingPipeline app(options);
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