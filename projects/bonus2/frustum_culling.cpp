#include <cstdio>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "frustum_culling.h"

const std::string planetRelPath = "obj/sphere.obj";
const std::string planetTextureRelPath = "texture/miscellaneous/planet_Quom1200.png";

const std::string asternoldRelPath = "obj/rock.obj";
const std::string asternoldTextureRelPath = "texture/miscellaneous/Rock-Texture-Surface.jpg";

FrustumCulling::FrustumCulling(const Options& options): Application(options) {
	// init model matrices
	initModelMatrices();

	// init model
	_planet.reset(new Model(getAssetFullPath(planetRelPath)));
	_planet->transform.scale = glm::vec3(10.0f, 10.0f, 10.0f);

	_asternoid.reset(new Model(getAssetFullPath(asternoldRelPath)));
	_instancedAsternoids.reset(new InstancedModel(getAssetFullPath(asternoldRelPath), _modelMatrices));

	// init textures
	auto planetTexture = std::make_shared<Texture2D>(getAssetFullPath(planetTextureRelPath));
	auto asternoidTexture = std::make_shared<Texture2D>(getAssetFullPath(asternoldTextureRelPath));

	// init materials
	_lineMaterial.reset(new LineMaterial);
	_lineMaterial->color = glm::vec3(0.0f, 1.0f, 0.0f);
	_lineMaterial->width = 1.0f;

	_planetMaterial.reset(new LambertMaterial);
	_planetMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
	_planetMaterial->mapKd = planetTexture;

	_asternoidMaterial.reset(new LambertMaterial);
	_asternoidMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
	_asternoidMaterial->mapKd = asternoidTexture;

	// init shaders
	initShaders();

	// init camera
	_camera.reset(new PerspectiveCamera(
		glm::radians(45.0f),
		1.0f * _windowWidth / _windowHeight,
		0.1f, 1000.0f));

	_camera->transform.position = glm::vec3(0.0f, 25.0f, 100.0f);
	_camera->transform.rotation = glm::angleAxis(-glm::radians(20.0f), _camera->transform.getRight());

	// init light
	_light.reset(new DirectionalLight());
	_light->transform.rotation = 
		glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

	// init visible array
	_visibles.resize(_amount, 1);

	// init gpu frustum culling resources
	initGPUCullingResources();

	// init indirect draw resources
	_indirectDrawCmds.reserve(_amount);
	glGenBuffers(1, &_indirectBuffer);

	// init imGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();
}

FrustumCulling::~FrustumCulling() {
	// destroy GPU frustum culling resources
	if (_transformFeedback) {
		glDeleteTransformFeedbacks(1, &_transformFeedback);
		_transformFeedback = 0;
	}

	if (_transformFeedbackResultBuffer) {
		glDeleteBuffers(1, &_transformFeedbackResultBuffer);
		_transformFeedbackResultBuffer = 0;
	}

	if (_transformFeedbackRenderVao) {
		glDeleteVertexArrays(1, &_transformFeedbackRenderVao);
		_transformFeedbackRenderVao = 0;
	}

	// destroy indirect draw resources
	if (_indirectBuffer) {
		glDeleteBuffers(1, &_indirectBuffer);
		_indirectBuffer = 0;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void FrustumCulling::initModelMatrices() {
	constexpr float radius = 50.0f;
	constexpr float offset = 10.0f;
	for (int i = 0; i < _amount; ++i) {
		glm::mat4 model(1.0f);
		// translate
		float angle = (float)i / (float)_amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.2f;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		// scale
		float scale = (rand() % 20) / 100.0f + 0.05f;
		model = glm::scale(model, glm::vec3(scale));

		// rotate
		float rotAngle = 1.0f * (rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		_modelMatrices.push_back(model);
	}
}

void FrustumCulling::initShaders() {
	const char* lineVsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"
		"void main() {\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"}\n";

	const char* lineVsInstancedCode = 
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in mat4 aInstanceMatrix;\n"
		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"void main() {\n"
		"	gl_Position = projection * view * aInstanceMatrix * vec4(aPosition, 1.0f);\n"
		"}\n";

	const char* lineFsCode =
		"#version 330 core\n"
		"out vec4 color;\n"
		"struct Material {\n"
		"	vec3 color;\n"
		"};\n"
		"uniform Material material;\n"
		"void main() { \n"
		"	color = vec4(material.color, 1.0f);\n"
		"}\n";

	_lineShader.reset(new GLSLProgram);
	_lineShader->attachVertexShader(lineVsCode);
	_lineShader->attachFragmentShader(lineFsCode);
	_lineShader->link();

	_lineInstancedShader.reset(new GLSLProgram);
	_lineInstancedShader->attachVertexShader(lineVsInstancedCode);
	_lineInstancedShader->attachFragmentShader(lineFsCode);
	_lineInstancedShader->link();

	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexture;\n"

		"out vec3 fPosition;\n"
		"out vec3 fNormal;\n"
		"out vec2 fTexCoord;\n"

		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"uniform mat4 model;\n"
		"void main() {\n"
		"	fPosition = vec3(model * vec4(aPosition, 1.0f));\n"
		"	fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
		"	fTexCoord = aTexture;\n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
		"}\n";

	const char* vsInstancedCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in vec3 aNormal;\n"
		"layout(location = 2) in vec2 aTexture;\n"
		"layout(location = 3) in mat4 aInstanceMatrix;\n"

		"out vec3 fPosition;\n"
		"out vec3 fNormal;\n"
		"out vec2 fTexCoord;\n"

		"uniform mat4 projection;\n"
		"uniform mat4 view;\n"
		"void main() {\n"
		"	fPosition = vec3(aInstanceMatrix * vec4(aPosition, 1.0f));\n"
		"	fNormal = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;\n"
		"	fTexCoord = aTexture;\n"
		"	gl_Position = projection * view * aInstanceMatrix * vec4(aPosition, 1.0f);\n"
		"}\n";

	const char* fsCode =
		"#version 330 core\n"
		"in vec3 fPosition;\n"
		"in vec3 fNormal;\n"
		"in vec2 fTexCoord;\n"
		"out vec4 color;\n"

		"struct Material {\n"
		"	vec3 kd;\n"
		"};\n"

		"struct DirectionalLight {\n"
		"	vec3 direction;\n"
		"	float intensity;\n"
		"	vec3 color;\n"
		"};\n"

		"uniform Material material;\n"
		"uniform sampler2D mapKd;\n"
		"uniform DirectionalLight light;\n"

		"void main() {\n"
		"	vec3 normal = normalize(fNormal);\n"
		"	vec3 lightDir = normalize(-light.direction);\n"
		"	vec3 diffuse = light.intensity * light.color * max(dot(lightDir, normal), 0.0f) * \n"
		"				   material.kd * texture(mapKd, fTexCoord).rgb;\n"
		"	color = vec4(diffuse, 1.0f);\n"
		"}\n";

	_lambertShader.reset(new GLSLProgram);
	_lambertShader->attachVertexShader(vsCode);
	_lambertShader->attachFragmentShader(fsCode);
	_lambertShader->link();

	_lambertInstancedShader.reset(new GLSLProgram);
	_lambertInstancedShader->attachVertexShader(vsInstancedCode);
	_lambertInstancedShader->attachFragmentShader(fsCode);
	_lambertInstancedShader->link();
}

void FrustumCulling::initGPUCullingResources() {
	// create transform feedback
	glGenTransformFeedbacks(1, &_transformFeedback);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, _transformFeedback);

	// create transform feedback result buffer
	glGenBuffers(1, &_transformFeedbackResultBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, _transformFeedbackResultBuffer);
	glBufferData(GL_ARRAY_BUFFER, _amount * sizeof(int), nullptr, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _transformFeedbackResultBuffer);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

	// create render objects
	glGenVertexArrays(1, &_transformFeedbackRenderVao);
	glBindVertexArray(_transformFeedbackRenderVao);
	
	glBindBuffer(GL_ARRAY_BUFFER, _instancedAsternoids->getInstacenVbo());

	constexpr GLsizei stride = sizeof(glm::mat4);
	constexpr GLsizei unitSize = sizeof(glm::vec4);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)(0 * unitSize));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(1 * unitSize));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(2 * unitSize));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * unitSize));

	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);

	glBindVertexArray(0);

	// TODO: Modify the following code to achieve GPU frustum culling
	// --------------------------------------------------------------------
	const char* vsCode =
		"#version 330 core\n"
		"layout(location = 0) in vec3 aPosition;\n"
		"layout(location = 1) in mat4 aInstanceMatrix;\n"
		"out int visible;\n"

		"struct BoundingBox {\n"
		"	vec3 min;\n"
		"	vec3 max;\n"
		"};\n"

		"struct Plane {\n"
		"	vec3 normal;\n"
		"	float signedDistance;\n"
		"};\n"

		"struct Frustum {\n"
		"	Plane planes[6];\n"
		"};\n"

		"uniform BoundingBox boundingBox;\n"
		"uniform Frustum frustum;\n"

		"void main() {\n"
		"	visible = 1;\n"
		"}\n";
	// --------------------------------------------------------------------

	// create frustum culling shader
	_frustumCullingShader.reset(new GLSLProgram);
	_frustumCullingShader->attachVertexShader(vsCode);
	_frustumCullingShader->setTransformFeedbackVaryings(
		{ "visible" }, GL_INTERLEAVED_ATTRIBS);
	_frustumCullingShader->link();
}

void FrustumCulling::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
		_camera->transform.position += _camera->transform.getFront() * _cameraMoveSpeed * (float)_deltaTime;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
		_camera->transform.position -= _camera->transform.getRight() * _cameraMoveSpeed * (float)_deltaTime;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
		_camera->transform.position -= _camera->transform.getFront() * _cameraMoveSpeed * (float)_deltaTime;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
		_camera->transform.position += _camera->transform.getRight() * _cameraMoveSpeed * (float)_deltaTime;
	}

	if (glMultiDrawElementsIndirect == nullptr) {
		_indirectDrawEnabled = false;
	}
}

void FrustumCulling::renderFrame() {
	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	const Frustum frustum = _camera->getFrustum();
	const glm::mat4 projection = _camera->getProjectionMatrix();
	const glm::mat4 view = _camera->getViewMatrix();

	// draw planet
	if (frustum.intersect(_planet->getBoundingBox(), _planet->transform.getLocalMatrix())) {
		_lambertShader->use();
		_lambertShader->setUniformMat4("projection", projection);
		_lambertShader->setUniformMat4("view", view);
		_lambertShader->setUniformMat4("model", _planet->transform.getLocalMatrix());
		_lambertShader->setUniformVec3("light.direction", _light->transform.getFront());
		_lambertShader->setUniformVec3("light.color", _light->color);
		_lambertShader->setUniformFloat("light.intensity", _light->intensity);
		_lambertShader->setUniformVec3("material.kd", _planetMaterial->kd);
		glActiveTexture(GL_TEXTURE0);
		_planetMaterial->mapKd->bind();

		_planet->draw();
	}

	// draw planet aabb
	if (_showBoundingBox) {
		_lineShader->use();
		_lineShader->setUniformMat4("projection", projection);
		_lineShader->setUniformMat4("view", view);
		_lineShader->setUniformMat4("model", _planet->transform.getLocalMatrix());
		_lineShader->setUniformVec3("material.color", _lineMaterial->color);
		glLineWidth(_lineMaterial->width);

		_planet->drawBoundingBox();
	}

	// test visiblity
	// results will be stored in std::vector<int> _visibles
	const BoundingBox box = _asternoid->getBoundingBox();
	switch (_method) {
	case Method::CPU:
		for (int i = 0; i < _amount; ++i) {
			_visibles[i] = static_cast<int>(frustum.intersect(box, _modelMatrices[i]));
		}
		break;
	case Method::GPU:
		// TODO: use the transform feedback to perform GPU frustum culling
		// write your code here
		// ------------------------------------------------------------------
		// _frustumCullingShader->use();
		// ------------------------------------------------------------------

		break;
	}

	if (_indirectDrawEnabled) {
		renderAsternoidsIndirect();
	} else {
		renderAsternoids();
	}

	// draw ui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Control Panel", nullptr, flags)) {
		ImGui::End();
	} else {
		ImGui::Text("render method");
		ImGui::Separator();
		ImGui::RadioButton("CPU", (int*)&_method, (int)(Method::CPU));
		ImGui::RadioButton("GPU", (int*)&_method, (int)(Method::GPU));

		ImGui::Checkbox("draw indirect", (bool*)&_indirectDrawEnabled);
		ImGui::Checkbox("show bounding box", (bool*)&_showBoundingBox);
		ImGui::NewLine();

		float fraction = 1.0f * _drawAsternoidCount / _amount;
		std::string fracInfo = std::to_string(_drawAsternoidCount) + "/" + std::to_string(_amount);
		ImGui::Text("visible fraction");
		ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f), fracInfo.c_str());
		ImGui::NewLine();

		std::string fpsInfo = "avg fps: " + std::to_string(_fpsIndicator.getAverageFrameRate());
		ImGui::Text("%s", fpsInfo.c_str());
		ImGui::PlotLines("", _fpsIndicator.getDataPtr(), _fpsIndicator.getSize(), 0,
			nullptr, 0.0f, std::numeric_limits<float>::max(), ImVec2(240.0f, 50.0f));

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void FrustumCulling::renderAsternoids() {
	_drawAsternoidCount = 0;

	const glm::mat4 projection = _camera->getProjectionMatrix();
	const glm::mat4 view = _camera->getViewMatrix();

	_lambertShader->use();
	_lambertShader->setUniformMat4("projection", projection);
	_lambertShader->setUniformMat4("view", view);
	_lambertShader->setUniformVec3("light.direction", _light->transform.getFront());
	_lambertShader->setUniformVec3("light.color", _light->color);
	_lambertShader->setUniformFloat("light.intensity", _light->intensity);
	_lambertShader->setUniformVec3("material.kd", _asternoidMaterial->kd);
	glActiveTexture(GL_TEXTURE0);
	_asternoidMaterial->mapKd->bind();

	for (int i = 0; i < _amount; ++i) {
		if (_visibles[i]) {
			_lambertShader->setUniformMat4("model", _modelMatrices[i]);
			_asternoid->draw();
			++_drawAsternoidCount;
		}
	}

	if (_showBoundingBox) {
		_lineShader->use();
		_lineShader->setUniformMat4("projection", projection);
		_lineShader->setUniformMat4("view", view);
		glLineWidth(_lineMaterial->width);
		_lineShader->setUniformVec3("material.color", _lineMaterial->color);

		for (int i = 0; i < _amount; ++i) {
			_lineShader->setUniformMat4("model", _modelMatrices[i]);
			_asternoid->drawBoundingBox();
		}
	}
}

void FrustumCulling::renderAsternoidsIndirect() {
	_drawAsternoidCount = 0;

	_indirectDrawCmds.clear();

	const glm::mat4 projection = _camera->getProjectionMatrix();
	const glm::mat4 view = _camera->getViewMatrix();
	const uint32_t count = static_cast<uint32_t>(_asternoid->getFaceCount() * 3);
	uint32_t instanceCount = 0;

	for (int i = 0; i < _amount; ++i) {
		if (_visibles[i]) {
			++instanceCount;
			++_drawAsternoidCount;
		} else {
			_indirectDrawCmds.push_back({ count, instanceCount, 0, 0, i - instanceCount });
			instanceCount = 0;
		}
	}

	if (instanceCount > 0) {
		_indirectDrawCmds.push_back({ count, instanceCount, 0, 0, _amount - instanceCount });
	}

	_lambertInstancedShader->use();
	_lambertInstancedShader->setUniformMat4("projection", projection);
	_lambertInstancedShader->setUniformMat4("view", view);
	_lambertInstancedShader->setUniformVec3("light.direction", _light->transform.getFront());
	_lambertInstancedShader->setUniformVec3("light.color", _light->color);
	_lambertInstancedShader->setUniformFloat("light.intensity", _light->intensity);
	_lambertInstancedShader->setUniformVec3("material.kd", _asternoidMaterial->kd);
	glActiveTexture(GL_TEXTURE0);
	_asternoidMaterial->mapKd->bind();

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, 
		_indirectDrawCmds.size() * sizeof(DrawElementsIndirectCommand),
		_indirectDrawCmds.data(), GL_STREAM_DRAW);

	glBindVertexArray(_instancedAsternoids->getVao());

	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, 
								static_cast<GLsizei>(_indirectDrawCmds.size()), 0);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);

	if (_showBoundingBox) {
		for (auto& cmd : _indirectDrawCmds) {
			cmd.count = 24;
		}

		_lineInstancedShader->use();
		_lineInstancedShader->setUniformMat4("projection", projection);
		_lineInstancedShader->setUniformMat4("view", view);
		glLineWidth(_lineMaterial->width);
		_lineInstancedShader->setUniformVec3("material.color", _lineMaterial->color);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuffer);
		glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, 
			_indirectDrawCmds.size() * sizeof(DrawElementsIndirectCommand), 
			_indirectDrawCmds.data());

		glBindVertexArray(_instancedAsternoids->getBoundingBoxVao());

		glMultiDrawElementsIndirect(GL_LINES, GL_UNSIGNED_INT, 0, 
									static_cast<GLsizei>(_indirectDrawCmds.size()), 0);

		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

		glBindVertexArray(0);
	}
}