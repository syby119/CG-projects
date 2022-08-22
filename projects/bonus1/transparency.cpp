#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "transparency.h"

const std::string knotPath = "../../media/knot.obj";
const std::string dragonPath = "../../media/dragon.obj";
const std::string transparentTexturePath = "../../media/transparent.png";

Transparency::Transparency(const Options& options) : Application(options) {
	// init models
	_knot.reset(new Model(knotPath));
	_knot->scale = glm::vec3(0.8f, 0.8f, 0.8f);

	// init light
	_light.reset(new DirectionalLight());
	_light->rotation = glm::angleAxis(glm::radians(45.0f), -glm::vec3(1.0f, 1.0f, 1.0f));

	// init camera
	_camera.reset(new PerspectiveCamera(
		glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 10000.0f));
	_camera->position.z = 10.0f;

	// init shaders
	initAlphaTestingShader();
	initAlphaBlendingShader();
	initDepthPeelingShaders();

	// init materials
	_knotMaterial.reset(new TransparentMaterial());
	_knotMaterial->albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	_knotMaterial->ka = 0.03f;
	_knotMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
	_knotMaterial->transparent = 0.8f;

	// init sphere texture
	_transparentTexture.reset(new Texture2D(transparentTexturePath));

	// init fullscreen quad
	_fullscreenQuad.reset(new FullscreenQuad);

	// init depth peeling resources
	initDepthPeelingResources();

	// init query
	glGenQueries(1, &_queryId);

	// init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();
}

Transparency::~Transparency() {
	if (_queryId) {
		glDeleteQueries(1, &_queryId);
		_queryId = 0;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Transparency::initAlphaTestingShader() {
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"

		"out vec3 fPos;\n"
		"out vec3 fNormal;\n"
		"out vec2 fTexCoord;\n"

		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"

		"void main() {\n"
		"	fPos = vec3(model * vec4(aPosition, 1.0f));\n"
		"	fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
		"	fTexCoord = aTexCoord;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"} \n";


	// TODO: modify the following code to achieve alpha testing algorithm
	// modify the code
	// ------------------------------------------------------------------
	const char* fsCode =
		"#version 330 core\n"
		"in vec3 fPos;\n"
		"in vec3 fNormal;\n"
		"in vec2 fTexCoord;\n"
		"out vec4 color;\n"

		"struct Material {\n"
		"	vec3 albedo;\n"
		"	float ka;\n"
		"	vec3 kd;\n"
		"	float transparent;\n"
		"};\n"

		"struct DirectionalLight {\n"
		"	vec3 direction;\n"
		"	float intensity;\n"
		"	vec3 color;\n"
		"};\n"

		"uniform Material material;\n"
		"uniform DirectionalLight directionalLight;\n"
		"uniform sampler2D transparentTexture;\n"

		"void main() {\n"
		"	vec4 texColor = texture(transparentTexture, fTexCoord);\n"
		"	vec3 normal = normalize(fNormal);\n"
		"	vec3 lightDir = normalize(-directionalLight.direction);\n"
		"	vec3 ambient = material.ka * material.albedo;\n"
		"	vec3 diffuse = material.kd * texColor.rgb * max(dot(lightDir, normal), 0.0f) * \n"
		"				   directionalLight.color * directionalLight.intensity;\n"
		"	color = vec4(ambient + diffuse, 1.0f);\n"
		"}\n";
	// ------------------------------------------------------------------

	_alphaTestingShader.reset(new GLSLProgram);
	_alphaTestingShader->attachVertexShader(vsCode);
	_alphaTestingShader->attachFragmentShader(fsCode);
	_alphaTestingShader->link();
}

void Transparency::initAlphaBlendingShader() {
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"

		"out vec3 fPos;\n"
		"out vec3 fNormal;\n"

		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"

		"void main() {\n"
		"	fPos = vec3(model * vec4(aPosition, 1.0f));\n"
		"	fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"} \n";

	const char* fsCode =
		"#version 330 core\n"
		"in vec3 fPos;\n"
		"in vec3 fNormal;\n"
		"out vec4 color;\n"

		"struct Material {\n"
		"	vec3 albedo;\n"
		"	float ka;\n"
		"	vec3 kd;\n"
		"	float transparent;\n"
		"};\n"

		"struct DirectionalLight {\n"
		"	vec3 direction;\n"
		"	float intensity;\n"
		"	vec3 color;\n"
		"};\n"

		"uniform Material material;\n"
		"uniform DirectionalLight directionalLight;\n"

		"void main() {\n"
		"	vec3 normal = normalize(fNormal);\n"
		"	vec3 lightDir = normalize(-directionalLight.direction);\n"
		"	vec3 ambient = material.ka * material.albedo ;\n"
		"	vec3 diffuse = material.kd * max(dot(lightDir, normal), 0.0f) * \n"
		"				   directionalLight.color * directionalLight.intensity;\n"
		"	color = vec4(ambient + diffuse, material.transparent);\n"
		"}\n";

	_alphaBlendingShader.reset(new GLSLProgram);
	_alphaBlendingShader->attachVertexShader(vsCode);
	_alphaBlendingShader->attachFragmentShader(fsCode);
	_alphaBlendingShader->link();
}

void Transparency::initDepthPeelingShaders() {
	const char* shadeVsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexCoord;\n"

		"out vec3 fPos;\n"
		"out vec3 fNormal;\n"

		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"

		"void main() {\n"
		"	fPos = vec3(model * vec4(aPosition, 1.0f));\n"
		"	fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"}\n";

	const char* blendVsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec2 aPosition;\n"
		"layout(location = 1) in vec2 aTexCoords;\n"
		"out vec2 fTexCoords;\n"
		"void main() {\n"
		"	fTexCoords = aTexCoords;\n"
		"	gl_Position = vec4(aPosition, 0.0f, 1.0f);\n"
		"}\n";

	const char* initFsCode =
		"#version 330 core\n"
		"in vec3 fPos;\n"
		"in vec3 fNormal;\n"
		"out vec4 color;\n"

		"struct Material {\n"
		"	vec3 albedo;\n"
		"	float ka;\n"
		"	vec3 kd;\n"
		"	float transparent;\n"
		"};\n"

		"struct DirectionalLight {\n"
		"	vec3 direction;\n"
		"	float intensity;\n"
		"	vec3 color;\n"
		"};\n"

		"uniform Material material;\n"
		"uniform DirectionalLight directionalLight;\n"

		"vec3 lambertShading() {\n"
		"	vec3 normal = normalize(fNormal);\n"
		"	vec3 lightDir = normalize(-directionalLight.direction);\n"
		"	vec3 ambient = material.ka * material.albedo ;\n"
		"	vec3 diffuse = material.kd * max(dot(lightDir, normal), 0.0f) * \n"
		"				   directionalLight.color * directionalLight.intensity;\n"
		"	return ambient + diffuse;\n"
		"}\n"

		"void main() {\n"
		"	vec3 premultiedColor = lambertShading() * material.transparent;\n"
		"	color = vec4(premultiedColor, 1.0f - material.transparent);\n"
		"}\n";

	const char* peelFsCode =
		"#version 330 core\n"
		"in vec3 fPos;\n"
		"in vec3 fNormal;\n"
		"out vec4 color;\n"

		"struct Material {\n"
		"	vec3 albedo;\n"
		"	float ka;\n"
		"	vec3 kd;\n"
		"	float transparent;\n"
		"};\n"

		"struct DirectionalLight {\n"
		"	vec3 direction;\n"
		"	float intensity;\n"
		"	vec3 color;\n"
		"};\n"

		"struct WindowExtent {\n"
		"	int width;\n"
		"	int height;\n"
		"};\n"

		"uniform Material material;\n"
		"uniform DirectionalLight directionalLight;\n"
		"uniform sampler2D depthTexture;\n"
		"uniform WindowExtent windowExtent;\n"

		"float getPeelingDepth() {\n"
		"	float u = gl_FragCoord.x / windowExtent.width;\n"
		"	float v = gl_FragCoord.y / windowExtent.height;\n"
		"	return texture(depthTexture, vec2(u, v)).r;\n"
		"}\n"

		"vec3 lambertShading() {\n"
		"	vec3 normal = normalize(fNormal);\n"
		"	vec3 lightDir = normalize(-directionalLight.direction);\n"
		"	vec3 ambient = material.ka * material.albedo ;\n"
		"	vec3 diffuse = material.kd * max(dot(lightDir, normal), 0.0f) * \n"
		"				   directionalLight.color * directionalLight.intensity;\n"
		"	return ambient + diffuse;\n"
		"}\n"

		"void main() {\n"
		"	if (gl_FragCoord.z <= getPeelingDepth()) {\n"
		"		discard;\n"
		"	}\n"

		"	vec3 premultiedColor = lambertShading() * material.transparent;\n"
		"	color = vec4(premultiedColor, material.transparent);\n"
		"}\n";

	const char* blendFsCode =
		"#version 330 core\n"
		"out vec4 color;\n"

		"struct WindowExtent {\n"
		"	int width;\n"
		"	int height;\n"
		"};\n"

		"uniform WindowExtent windowExtent;\n"
		"uniform sampler2D blendTexture;\n"

		"void main() {\n"
		"	float u = gl_FragCoord.x / windowExtent.width;\n"
		"	float v = gl_FragCoord.y / windowExtent.height;\n"
		"	color = texture(blendTexture, vec2(u, v));\n"
		"}\n";

	const char* finalFsCode =
		"#version 330 core\n"
		"out vec4 color;\n"

		"struct WindowExtent {\n"
		"	int width;\n"
		"	int height;\n"
		"};\n"

		"uniform WindowExtent windowExtent;\n"
		"uniform sampler2D blendTexture;\n"
		"uniform vec4 backgroundColor;\n"

		"void main() {\n"
		"	float u = gl_FragCoord.x / windowExtent.width;\n"
		"	float v = gl_FragCoord.y / windowExtent.height;\n"
		"	vec4 frontColor = texture(blendTexture, vec2(u, v));\n"
		"	color = frontColor + backgroundColor * frontColor.a;\n"
		"}\n";

	_depthPeelingInitShader.reset(new GLSLProgram);
	_depthPeelingInitShader->attachVertexShader(shadeVsCode);
	_depthPeelingInitShader->attachFragmentShader(initFsCode);
	_depthPeelingInitShader->link();

	_depthPeelingShader.reset(new GLSLProgram);
	_depthPeelingShader->attachVertexShader(shadeVsCode);
	_depthPeelingShader->attachFragmentShader(peelFsCode);
	_depthPeelingShader->link();

	_depthPeelingBlendShader.reset(new GLSLProgram);
	_depthPeelingBlendShader->attachVertexShader(blendVsCode);
	_depthPeelingBlendShader->attachFragmentShader(blendFsCode);
	_depthPeelingBlendShader->link();

	_depthPeelingFinalShader.reset(new GLSLProgram);
	_depthPeelingFinalShader->attachVertexShader(blendVsCode);
	_depthPeelingFinalShader->attachFragmentShader(finalFsCode);
	_depthPeelingFinalShader->link();
}

void Transparency::initDepthPeelingResources() {
	for (int i = 0; i < 2; ++i) {
		_fbos[i].reset(new Framebuffer);
		_colorTextures[i].reset(
			new DataTexture(GL_RGBA, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));
		_depthTextures[i].reset(
			new DataTexture(GL_DEPTH_COMPONENT32F, _windowWidth, _windowHeight, GL_DEPTH_COMPONENT, GL_FLOAT));

		_fbos[i]->bind();
		_fbos[i]->attach(*_colorTextures[i], GL_COLOR_ATTACHMENT0);
		_fbos[i]->attach(*_depthTextures[i], GL_DEPTH_ATTACHMENT);
		_fbos[i]->unbind();
	}

	_colorBlendTexture.reset(new DataTexture(GL_RGBA, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));
	_colorBlendFbo.reset(new Framebuffer);
	_colorBlendFbo->bind();
	_colorBlendFbo->attach(*_colorBlendTexture, GL_COLOR_ATTACHMENT0);
	_colorBlendFbo->attach(*_depthTextures[0], GL_DEPTH_ATTACHMENT);
	_colorBlendFbo->unbind();
}

void Transparency::handleInput() {
	const float angluarVelocity = 0.1f;
	const float angle = angluarVelocity * static_cast<float>(_deltaTime);
	const glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
	_knot->rotation = glm::angleAxis(angle, axis) * _knot->rotation;
}

void Transparency::renderFrame() {
	// trivial things
	showFpsInWindowTitle();

	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	switch (_renderMode) {
	case RenderMode::AlphaTesting:
		renderWithAlphaTesting();
		break;
	case RenderMode::AlphaBlending:
		renderWithAlphaBlending();
		break;
	case RenderMode::DepthPeeling:
		renderWithDepthPeeling();
		break;
	}

	// draw ui elements
	renderUI();
}

void Transparency::renderWithAlphaTesting() {
	_alphaTestingShader->use();
	// 1 set transformation matrices
	_alphaTestingShader->setMat4("projection", _camera->getProjectionMatrix());
	_alphaTestingShader->setMat4("view", _camera->getViewMatrix());
	_alphaTestingShader->setMat4("model", _knot->getModelMatrix());
	// 2 set light
	_alphaTestingShader->setVec3("directionalLight.direction", _light->getFront());
	_alphaTestingShader->setFloat("directionalLight.intensity", _light->intensity);
	_alphaTestingShader->setVec3("directionalLight.color", _light->color);
	// 3 set material
	_alphaTestingShader->setVec3("material.albedo", _knotMaterial->albedo);
	_alphaTestingShader->setFloat("material.ka", _knotMaterial->ka);
	_alphaTestingShader->setVec3("material.kd", _knotMaterial->kd);
	_alphaTestingShader->setFloat("material.transparent", _knotMaterial->transparent);
	// 4 set texture
	glActiveTexture(GL_TEXTURE0);
	_transparentTexture->bind();

	_knot->draw();
}

void Transparency::renderWithAlphaBlending() {
	//  render transparent objects
	_alphaBlendingShader->use();
	// 1 set transformation matrices
	_alphaBlendingShader->setMat4("projection", _camera->getProjectionMatrix());
	_alphaBlendingShader->setMat4("view", _camera->getViewMatrix());
	_alphaBlendingShader->setMat4("model", _knot->getModelMatrix());
	// 2 set light
	_alphaBlendingShader->setVec3("directionalLight.direction", _light->getFront());
	_alphaBlendingShader->setFloat("directionalLight.intensity", _light->intensity);
	_alphaBlendingShader->setVec3("directionalLight.color", _light->color);
	// 3 set material
	_alphaBlendingShader->setVec3("material.albedo", _knotMaterial->albedo);
	_alphaBlendingShader->setFloat("material.ka", _knotMaterial->ka);
	_alphaBlendingShader->setVec3("material.kd", _knotMaterial->kd);
	_alphaBlendingShader->setFloat("material.transparent", _knotMaterial->transparent);

	// TODO: use two render passes to achieve alpha blending
	// pass 1: Write the depth info to the zbuffer, while leave the color buffer unmodified.
	//         This pass will record the depth info of the object to avoid backward part to
	//         be rendered.
	// write your code here
	// ------------------------------------------------------------------------
	// ...
	// ------------------------------------------------------------------------

	_knot->draw();

	// pass 2: Write the color buffer using the zbuffer info from pass 1 with blending, 
	//		   while leave the depth buffer unmodified.
	// write your code here
	// ------------------------------------------------------------------------
	// ...
	// ------------------------------------------------------------------------

	_knot->draw();
	// restore: don't forget to restore OpenGL state before pass 1, which will avoid side effects
	//          to the object rendering afterwards.
	// write your code here
	// ------------------------------------------------------------------------
	// ...
	// ------------------------------------------------------------------------

}

void Transparency::renderWithDepthPeeling() {
	const glm::mat4 projection = _camera->getProjectionMatrix();
	const glm::mat4 view = _camera->getViewMatrix();

	// 1. initialize min depth buffer
	_colorBlendFbo->bind();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	_depthPeelingInitShader->use();
	// 1.1 set transformation matrices
	_depthPeelingInitShader->setMat4("projection", projection);
	_depthPeelingInitShader->setMat4("view", view);
	_depthPeelingInitShader->setMat4("model", _knot->getModelMatrix());
	// 1.2 set light
	_depthPeelingInitShader->setVec3("directionalLight.direction", _light->getFront());
	_depthPeelingInitShader->setFloat("directionalLight.intensity", _light->intensity);
	_depthPeelingInitShader->setVec3("directionalLight.color", _light->color);
	// 1.3 set material
	_depthPeelingInitShader->setVec3("material.albedo", _knotMaterial->albedo);
	_depthPeelingInitShader->setFloat("material.ka", _knotMaterial->ka);
	_depthPeelingInitShader->setVec3("material.kd", _knotMaterial->kd);
	_depthPeelingInitShader->setFloat("material.transparent", _knotMaterial->transparent);

	_knot->draw();

	// 2. TODO: depth peeling and blending
	// hint1: this stage can be divided into iterative 2 pass: peeling pass and blending pass
	// hint2: use _fbos as ping-pong framebuffer for peeling pass
	// hint3: use _depthPeelingShader for peeling pass
	// hint4: use _colorBlendFbo for blending pass
	// hint5: use _depthPeelingBlendShader for blend pass
	// hint6: you can use glBeginQuery / glEndQuery / glGetQueryObjectuiv to end looping.
	// hint7: if it is to difficult for you, just use a predefined MAX_LAYER_NUM to end looping
	// write your code here
	// ------------------------------------------------------------------------
	// for (int layer = 1; layer < MAX_LAYER_NUM; ++layer) {
	// 	   // 2.1 peeling pass
	// 	   // 2.2 blending pass
	// }
	// ------------------------------------------------------------------------

	// 3. final pass: blend the peeling result with the background color
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_DEPTH_TEST);
	_depthPeelingFinalShader->use();
	// 3.1 set window extent
	_depthPeelingFinalShader->setInt("windowExtent.width", _windowWidth);
	_depthPeelingFinalShader->setInt("windowExtent.height", _windowHeight);
	// 3.2 set blend texture
	glActiveTexture(GL_TEXTURE0);
	_colorBlendTexture->bind();
	// 3.3 set background color
	_depthPeelingFinalShader->setVec4("backgroundColor", _clearColor);

	_fullscreenQuad->draw();
}

void Transparency::renderUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Control Panel", nullptr, flags)) {
		ImGui::End();
	}
	else {
		ImGui::Text("Render Mode");
		ImGui::Separator();
		ImGui::RadioButton("Alpha Testing", (int*)&_renderMode, (int)(RenderMode::AlphaTesting));
		ImGui::RadioButton("Alpha Blending", (int*)&_renderMode, (int)(RenderMode::AlphaBlending));
		ImGui::RadioButton("Depth Peeling", (int*)&_renderMode, (int)(RenderMode::DepthPeeling));
		ImGui::SliderFloat("transparent", &_knotMaterial->transparent, 0.0f, 1.0f);
		ImGui::NewLine();

		ImGui::ColorEdit3("background", (float*)&_clearColor);

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}