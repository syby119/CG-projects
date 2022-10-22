#include <iostream>
#include <cstdlib>

#include "raytracing.h"

Options getOptions(int argc, char* argv[]) {
	Options options;
	options.windowTitle = "RayTracing";
	// TODO: change appropriate resolution for debugging
	options.windowWidth = 640;
	options.windowHeight = 360;
	options.windowResizable = false;
	options.vSync = true;
	options.msaa = false;
	options.glVersion = { 3, 3 };
	options.backgroundColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	options.assetRootDir = "../../media/";

	return options;
}

int main(int argc, char* argv[]) {
	Options options = getOptions(argc, argv);

	try {
		RayTracing app(options);
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