#include <iostream>
#include <cstdlib>

#include "frustum_culling.h"

Options getOptions(int argc, char* argv[]) {
	Options options;
	options.windowTitle = "Frustum Culling";
	options.windowWidth = 1280;
	options.windowHeight = 720;
	options.windowResizable = false;
	options.vSync = true;
	options.msaa = true;
#ifdef _WIN32
	options.glVersion = { 4, 3 };
#else
	options.glVersion = { 4, 0 };
#endif
	options.backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	options.assetRootDir = "../../media/";

	return options;
}

int main(int argc, char* argv[]) {
	Options options = getOptions(argc, argv);

	try {
		FrustumCulling app(options);
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