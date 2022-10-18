#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "texture_mapping.h"

const std::string modelRelPath = "obj/sphere.obj";

const std::string earthTextureRelPath = "texture/miscellaneous/earthmap.jpg";
const std::string planetTextureRelPath = "texture/miscellaneous/planet_Quom1200.png";

const std::vector<std::string> skyboxTextureRelPaths = {
	"texture/skybox/Right_Tex.jpg",
	"texture/skybox/Left_Tex.jpg",
	"texture/skybox/Up_Tex.jpg",
	"texture/skybox/Down_Tex.jpg",
	"texture/skybox/Front_Tex.jpg",
	"texture/skybox/Back_Tex.jpg"
};

TextureMapping::TextureMapping(const Options& options): Application(options) {
	// init model
	_sphere.reset(new Model(getAssetFullPath(modelRelPath)));
	_sphere->transform.scale = glm::vec3(3.0f, 3.0f, 3.0f);

	// init textures
	std::shared_ptr<Texture2D> earthTexture = 
		std::make_shared<ImageTexture2D>(getAssetFullPath(earthTextureRelPath));
	std::shared_ptr<Texture2D> planetTexture = 
		std::make_shared<ImageTexture2D>(getAssetFullPath(planetTextureRelPath));

	// init materials
	_simpleMaterial.reset(new SimpleMaterial);
	_simpleMaterial->mapKd = planetTexture;

	_blendMaterial.reset(new BlendMaterial);
	_blendMaterial->kds[0] = glm::vec3(1.0f, 1.0f, 1.0f);
	_blendMaterial->kds[1] = glm::vec3(1.0f, 1.0f, 1.0f);
	_blendMaterial->mapKds[0] = planetTexture;
	_blendMaterial->mapKds[1] = earthTexture;
	_blendMaterial->blend = 0.0f;

	_checkerMaterial.reset(new CheckerMaterial);
	_checkerMaterial->repeat = 10;
	_checkerMaterial->colors[0] = glm::vec3(1.0f, 1.0f, 1.0f);
	_checkerMaterial->colors[1] = glm::vec3(0.0f, 0.0f, 0.0f);

	// init skybox
	std::vector<std::string> skyboxTextureFullPaths;
	for (size_t i = 0; i < skyboxTextureRelPaths.size(); ++i) {
		skyboxTextureFullPaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
	}
	_skybox.reset(new SkyBox(skyboxTextureFullPaths));

	// init camera
	_camera.reset(new PerspectiveCamera(
		glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 10000.0f));
	_camera->transform.position.z = 10.0f;

	// init light
	_light.reset(new DirectionalLight());
	_light->transform.rotation = 
		glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

	// init shaders
	initSimpleShader();
	initBlendShader();
	initCheckerShader();

	// init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();
}

TextureMapping::~TextureMapping() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void TextureMapping::initSimpleShader() {
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"
		"out vec2 fTexCoord;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"

		"void main() {\n"
		"	fTexCoord = aTexCoord;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"}\n";

	const char* fsCode =
		"#version 330 core\n"
		"in vec2 fTexCoord;\n"
		"out vec4 color;\n"
		"uniform sampler2D mapKd;\n"
		"void main() {\n"
		"	color = texture(mapKd, fTexCoord);\n"
		"}\n";

	_simpleShader.reset(new GLSLProgram); 
	_simpleShader->attachVertexShader(vsCode);
	_simpleShader->attachFragmentShader(fsCode);
	_simpleShader->link();
}

void TextureMapping::initBlendShader() {
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"
		"out vec3 fPosition;\n"
		"out vec3 fNormal;\n"
		"out vec2 fTexCoord;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"

		"void main() {\n"
		"	fPosition = vec3(model * vec4(aPosition, 1.0f));\n"
		"	fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
		"	fTexCoord = aTexCoord;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"}\n";

	// TODO: change the fragment shader code to achieve the following goals
	// + blend the colors of the two textures
	// + lambert shading, i.e the color is affected by the light
	// write your code here
	// -----------------------------------------------------------------
	const char* fsCode =
		"#version 330 core\n"
		"in vec3 fPosition;\n"
		"in vec3 fNormal;\n"
		"in vec2 fTexCoord;\n"
		"out vec4 color;\n"

		"struct DirectionalLight {\n"
		"	vec3 direction;\n"
		"	vec3 color;\n"
		"	float intensity;\n"
		"};\n"

		"struct Material {\n"
		"	vec3 kds[2];\n"
		"	float blend;\n"
		"};\n"

		"uniform Material material;\n"
		"uniform DirectionalLight light;\n"
		"uniform sampler2D mapKds[2];\n"

		"void main() {\n"
		"	color = texture(mapKds[0], fTexCoord);\n"
		"}\n";
	//----------------------------------------------------------------

	_blendShader.reset(new GLSLProgram);
	_blendShader->attachVertexShader(vsCode);
	_blendShader->attachFragmentShader(fsCode);
	_blendShader->link();
}

void TextureMapping::initCheckerShader() {
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"
		"out vec2 fTexCoord;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"
		"void main() {\n"
		"	fTexCoord = aTexCoord;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"}\n";

	// TODO: change the following code to achieve the procedural checker texture
	// hint: use the fTexCoord to determine the color
	// modify your code here
	// --------------------------------------------------------------
	const char* fsCode =
		"#version 330 core\n"
		"in vec2 fTexCoord;\n"
		"out vec4 color;\n"

		"struct Material {\n"
		"	vec3 colors[2];\n"
		"	int repeat;\n"
		"};\n"

		"uniform Material material;\n"

		"void main() {\n"
		"	color = vec4(material.colors[0], 1.0f);\n"
		"}\n";
	//----------------------------------------------------------------

	_checkerShader.reset(new GLSLProgram);
	_checkerShader->attachVertexShader(vsCode);
	_checkerShader->attachFragmentShader(fsCode);
	_checkerShader->link();
}

void TextureMapping::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return ;
	}

	const float angluarVelocity = 0.1f;
	const float angle = angluarVelocity * static_cast<float>(_deltaTime);
	const glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
	_sphere->transform.rotation = glm::angleAxis(angle, axis) * _sphere->transform.rotation;
}

void TextureMapping::renderFrame() {
	// some options related to imGUI
	static bool wireframe = false;
	
	// trivial things
	showFpsInWindowTitle();

	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	const glm::mat4 projection = _camera->getProjectionMatrix();
	const glm::mat4 view = _camera->getViewMatrix();

	// draw planet
	switch (_renderMode) {
	case RenderMode::Simple:
		// 1. use the shader
		_simpleShader->use();
		// 2. transfer mvp matrices to gpu 
		_simpleShader->setUniformMat4("projection", projection);
		_simpleShader->setUniformMat4("view", view);
		_simpleShader->setUniformMat4("model", _sphere->transform.getLocalMatrix());
		// 3. enable textures and transform textures to gpu
		_simpleMaterial->mapKd->bind();
		break;
	case RenderMode::Blend:
		// 1. use the shader
		_blendShader->use();
		// 2. transfer mvp matrices to gpu 
		_blendShader->setUniformMat4("projection", projection);
		_blendShader->setUniformMat4("view", view);
		_blendShader->setUniformMat4("model", _sphere->transform.getLocalMatrix());
		// 3. transfer light attributes to gpu
		_blendShader->setUniformVec3("light.direction", _light->transform.getFront());
		_blendShader->setUniformVec3("light.color", _light->color);
		_blendShader->setUniformFloat("light.intensity", _light->intensity);
		// 4. transfer materials to gpu
		// 4.1 transfer simple material attributes
		_blendShader->setUniformVec3("material.kds[0]", _blendMaterial->kds[0]);
		_blendShader->setUniformVec3("material.kds[1]", _blendMaterial->kds[1]);
		// 4.2 transfer blend cofficient to gpu
		_blendShader->setUniformFloat("material.blend", _blendMaterial->blend);
		// 4.3 TODO: enable textures and transform textures to gpu
		// write your code here
		//----------------------------------------------------------------
		// ...
		//----------------------------------------------------------------
		break;
	case RenderMode::Checker:
		// 1. use the shader
		_checkerShader->use();
		// 2. transfer mvp matrices to gpu 
		_checkerShader->setUniformMat4("projection", projection);
		_checkerShader->setUniformMat4("view", view);
		_checkerShader->setUniformMat4("model", _sphere->transform.getLocalMatrix());
		// 3. transfer material attributes to gpu
		_checkerShader->setUniformInt("material.repeat", _checkerMaterial->repeat);
		_checkerShader->setUniformVec3("material.colors[0]", _checkerMaterial->colors[0]);
		_checkerShader->setUniformVec3("material.colors[1]", _checkerMaterial->colors[1]);
		break;
	}

	_sphere->draw();

	// draw skybox
	_skybox->draw(projection, view);

	// draw ui elements
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Control Panel", nullptr, flags)) {
		ImGui::End();
	} else {
		ImGui::Text("Render Mode");
		ImGui::Separator();
		ImGui::RadioButton("Simple Texture Shading", (int*)&_renderMode, (int)(RenderMode::Simple));
		ImGui::NewLine();

		ImGui::RadioButton("Blend Texture Shading", (int*)&_renderMode, (int)(RenderMode::Blend));
		ImGui::ColorEdit3("kd1", (float*)&_blendMaterial->kds[0]);
		ImGui::ColorEdit3("kd2", (float*)&_blendMaterial->kds[1]);
		ImGui::SliderFloat("blend", &_blendMaterial->blend, 0.0f, 1.0f);
		ImGui::NewLine();

		ImGui::RadioButton("Checker Shading", (int*)&_renderMode, (int)(RenderMode::Checker));
		ImGui::SliderInt("repeat", &_checkerMaterial->repeat, 2, 20);
		ImGui::ColorEdit3("color1", (float*)&_checkerMaterial->colors[0]);
		ImGui::ColorEdit3("color2", (float*)&_checkerMaterial->colors[1]);
		ImGui::Checkbox("wireframe", &wireframe);
		ImGui::NewLine();

		ImGui::Text("Directional light");
		ImGui::Separator();
		ImGui::SliderFloat("intensity", &_light->intensity, 0.0f, 2.0f);
		ImGui::ColorEdit3("color", (float*)&_light->color);
		ImGui::NewLine();

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}