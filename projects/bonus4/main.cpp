#include <iostream>
#include <cstdlib>
#include "shadow_mapping.h"

Options getOptions(int argc, char* argv[]) {
	Options options;
	options.windowTitle = "Shadow Mapping";
	options.windowWidth = 1280;
	options.windowHeight = 720;
	options.windowResizable = false;
	options.vSync = true;
	options.msaa = true;
	options.glVersion = { 4, 0 };
	options.backgroundColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	options.assetRootDir = "../../media/";

	return options;
}

int main(int argc, char* argv[]) {
	Options options = getOptions(argc, argv);

	try {
		ShadowMapping app(options);
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