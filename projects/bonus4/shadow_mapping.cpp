#include <iostream>
#include <limits>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shadow_mapping.h"

constexpr int shadowMapResolution = 4096;

const std::string bunnyRelPath = "obj/bunny.obj";
const std::string arrowRelPath = "obj/arrow.obj";
const std::string sphereRelPath = "obj/sphere.obj";
const std::string cubeRelPath = "obj/cube.obj";

const std::string lightVsRelPath = "shader/bonus4/light.vert";
const std::string lightFsRelPath = "shader/bonus4/light.frag";

const std::string directionalDepthVsRelPath = "shader/bonus4/directional_depth.vert";
const std::string directionalDepthFsRelPath = "shader/bonus4/directional_depth.frag";

const std::string quadVsRelPath = "shader/bonus4/quad.vert";
const std::string quadFsRelPath = "shader/bonus4/quad.frag";

const std::string omnidirectionalDepthVsRelPath = "shader/bonus4/omnidirectional_depth.vert";
const std::string omnidirectionalDepthGsRelPath = "shader/bonus4/omnidirectional_depth.geom";
const std::string omnidirectionalDepthFsRelPath = "shader/bonus4/omnidirectional_depth.frag";

const std::string cubeVsRelPath = "shader/bonus4/cube.vert";
const std::string cubeFsRelPath = "shader/bonus4/cube.frag";

const std::string directionalDepthCsmVsRelPath = "shader/bonus4/directional_depth_csm.vert";
const std::string directionalDepthCsmGsRelPath = "shader/bonus4/directional_depth_csm.geom";
const std::string directionalDepthCsmFsRelPath = "shader/bonus4/directional_depth_csm.frag";

const std::string quadCsmVsRelPath = "shader/bonus4/quad_csm.vert";
const std::string quadCsmFsRelPath = "shader/bonus4/quad_csm.frag";

const std::string lambertVsRelPath = "shader/bonus4/lambert.vert";
const std::string lambertFsRelPath = "shader/bonus4/lambert.frag";

ShadowMapping::ShadowMapping(const Options& options) : Application(options) {
	// init bunnies
	for (int i = 0; i < 9; ++i) {
		if (i == 0) {
			_bunnies.emplace_back(new Model(getAssetFullPath(bunnyRelPath)));
		} else {
			_bunnies.emplace_back(new Model(_bunnies[0]->getVertices(), _bunnies[0]->getIndices()));
		}
		_bunnies[i]->transform.position.y = 2.5f;
		_bunnies[i]->transform.position.x = 20.0f * (i % 3 - 1);
		_bunnies[i]->transform.position.z = 20.0f * (i / 3 - 1);
	}
	_bunnyMaterial.reset(new LambertMaterial);
	_bunnyMaterial->kd = glm::vec3(1.0f);

	// init arrow and sphere for light representations
	_arrow.reset(new Model(getAssetFullPath(arrowRelPath)));
	_sphere.reset(new Model(getAssetFullPath(sphereRelPath)));

	//init ground
	initGround();

	// init camera
	_camera.reset(new PerspectiveCamera(
		glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 1000.0f));
	_camera->transform.position.y = 12.0f;
	_camera->transform.position.z = 12.0f;
	_camera->fovy = glm::radians(35.0f);
	_camera->transform.lookAt(glm::vec3(0.0f, 0.0f, -2.0f));

	// init lights
	_ambientLight.reset(new AmbientLight);

	_directionalLight.reset(new DirectionalLight);
	_directionalLight->transform.position = glm::vec3(-10.0f, 10.0f, 10.0f);
	_directionalLight->transform.scale = glm::vec3(1.0f, 1.0f, 2.0f);
	_directionalLight->transform.lookAt(glm::vec3(0.0f));
	_directionalLight->intensity = 0.5f;

	updateDirectionalLightSpaceMatrix();

	updateDirectionalLightSpaceMatrices();

	_pointLight.reset(new PointLight);
	_pointLight->transform.position = glm::vec3(-7.5f, 8.0f, 6.0f);
	_pointLight->transform.scale = glm::vec3(0.2f, 0.2f, 0.2f);
	_pointLight->intensity = 1.0f;
	_pointLight->kl = 0.1f;
	_pointLight->kq = 0.0f;

	updatePointLightSpaceMatrices();

	// init shaders
	initShaders();

	// init framebuffer
	_framebuffer.reset(new Framebuffer);
	_framebuffer->bind();
	_framebuffer->drawBuffer(GL_NONE);
	_framebuffer->readBuffer(GL_NONE);
	_framebuffer->unbind();

	std::vector<float> borderColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	// init depth texture
	_depthTexture.reset(new Texture2D(
		GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, GL_DEPTH_COMPONENT, GL_FLOAT));

	_depthTexture->bind();
	_depthTexture->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_depthTexture->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_depthTexture->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_depthTexture->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	_depthTexture->setParamterFloatVector(GL_TEXTURE_BORDER_COLOR, borderColor);
	_depthTexture->unbind();

	// init depth cube texture
	_depthCubeTexture.reset(new TextureCubemap(
		GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, GL_DEPTH_COMPONENT, GL_FLOAT));

	_depthCubeTexture->bind();
	_depthCubeTexture->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_depthCubeTexture->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_depthCubeTexture->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	_depthCubeTexture->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	_depthCubeTexture->setParamterInt(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	_depthCubeTexture->unbind();

	// init depth texture array
	_depthTextureArray.reset(new Texture2DArray(
		GL_DEPTH_COMPONENT32, shadowMapResolution, shadowMapResolution,
		static_cast<int>(_directionalLightSpaceMatrices.size()), GL_DEPTH_COMPONENT, GL_FLOAT));

	_depthTextureArray->bind();
	_depthTextureArray->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_depthTextureArray->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_depthTextureArray->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_depthTextureArray->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	_depthTextureArray->setParamterFloatVector(GL_TEXTURE_BORDER_COLOR, borderColor);
	_depthTextureArray->unbind();

	// init quad and cube for debug
	_quad.reset(new FullscreenQuad);
	_cube.reset(new Model(getAssetFullPath(cubeRelPath)));

	// init imGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();

	// render a frame ahead of time to fix the bug in Ubuntu
	_enableCascadeShadowMapping = true;
	renderFrame();
	_enableCascadeShadowMapping = false;
}

ShadowMapping::~ShadowMapping() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ShadowMapping::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return ;
	}

	constexpr float cameraMoveSpeed = 50.0f;
	if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
		_camera->transform.position += cameraMoveSpeed * _camera->transform.getFront() * _deltaTime;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
		_camera->transform.position -= cameraMoveSpeed * _camera->transform.getRight() * _deltaTime;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
		_camera->transform.position -= cameraMoveSpeed * _camera->transform.getFront() * _deltaTime;
	}

	if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
		_camera->transform.position += cameraMoveSpeed * _camera->transform.getRight() * _deltaTime;
	}

	constexpr float lightMoveSpeed = 5.0f;
	bool lightMoved = false;
	if (_input.keyboard.keyStates[GLFW_KEY_UP] != GLFW_RELEASE) {
		lightMoved = true;
		_directionalLight->transform.position +=
			lightMoveSpeed * _directionalLight->transform.getFront() * _deltaTime;
		_directionalLight->transform.lookAt(glm::vec3(0.0f));
	}

	if (_input.keyboard.keyStates[GLFW_KEY_DOWN] != GLFW_RELEASE) {
		lightMoved = true;
		_directionalLight->transform.position -=
			lightMoveSpeed * _directionalLight->transform.getFront() * _deltaTime;
		_directionalLight->transform.lookAt(glm::vec3(0.0f));
	}

	if (_input.keyboard.keyStates[GLFW_KEY_LEFT] != GLFW_RELEASE) {
		lightMoved = true;
		_directionalLight->transform.position -=
			lightMoveSpeed * _directionalLight->transform.getRight() * _deltaTime;
		_directionalLight->transform.lookAt(glm::vec3(0.0f));
	}

	if (_input.keyboard.keyStates[GLFW_KEY_RIGHT] != GLFW_RELEASE) {
		lightMoved = true;
		_directionalLight->transform.position +=
			lightMoveSpeed * _directionalLight->transform.getRight() * _deltaTime;
		_directionalLight->transform.lookAt(glm::vec3(0.0f));
	}

	if (lightMoved) {
		updateDirectionalLightSpaceMatrix();
	}

	updateDirectionalLightSpaceMatrices();

	_input.forwardState();
}

void ShadowMapping::renderFrame() {
	showFpsInWindowTitle();

	glEnable(GL_DEPTH_TEST);

	renderDirectionalLightShadowMap();
	renderPointLightShadowMap();

	renderScene();

	renderDebugView();

	renderUI();
}

void ShadowMapping::initGround() {
	constexpr float infinity = 100.0f;
	std::vector<Vertex>	vertices(4);
	for (size_t i = 0; i < vertices.size(); ++i) {
		vertices[i].position.x = (i % 2) ? infinity : -infinity;
		vertices[i].position.y = 0.0f;
		vertices[i].position.z = (i > 1) ? infinity : -infinity;
		vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
		vertices[i].texCoord.x = (i % 2) ? 1.0f : 0.0f;
		vertices[i].texCoord.y = (i > 1) ? 1.0f : 0.0f;
	};

	std::vector<uint32_t> indices = {
		0, 1, 2, 1, 2, 3
	};

	_ground.reset(new Model(vertices, indices));

	_groundMaterial.reset(new LambertMaterial);
	_groundMaterial->kd = glm::vec3(0.8f);
}

void ShadowMapping::initShaders() {
	initDirectionalDepthShader();
	initOmnidirectionalDepthShader();
	initDirectionalCascadeDepthShader();

	initLambertShader();
	initLightShader();

	initQuadShader();
	initCubeShader();
	initQuadCascadeShader();
}

void ShadowMapping::initLambertShader() {
	// TODO: change the lambert.frag code to render soft shadows, including
	// + shadow mapping for the directional light
	// + omnidirectional shadow mapping for the point light
	// + cascade shadow mapping for the directional light
	_lambertShader.reset(new GLSLProgram);
	_lambertShader->attachVertexShaderFromFile(getAssetFullPath(lambertVsRelPath));
	_lambertShader->attachFragmentShaderFromFile(getAssetFullPath(lambertFsRelPath));
	_lambertShader->link();
}

void ShadowMapping::initLightShader() {
	_lightShader.reset(new GLSLProgram);
	_lightShader->attachVertexShaderFromFile(getAssetFullPath(lightVsRelPath));
	_lightShader->attachFragmentShaderFromFile(getAssetFullPath(lightFsRelPath));
	_lightShader->link();
}

void ShadowMapping::initDirectionalDepthShader() {
	_directionalDepthShader.reset(new GLSLProgram);
	_directionalDepthShader->attachVertexShaderFromFile(getAssetFullPath(directionalDepthVsRelPath));
	_directionalDepthShader->attachFragmentShaderFromFile(getAssetFullPath(directionalDepthFsRelPath));
	_directionalDepthShader->link();
}

void ShadowMapping::initOmnidirectionalDepthShader() {
	_omnidirectionalDepthShader.reset(new GLSLProgram);

	_omnidirectionalDepthShader->attachVertexShaderFromFile(
		getAssetFullPath(omnidirectionalDepthVsRelPath));
	_omnidirectionalDepthShader->attachGeometryShaderFromFile(
		getAssetFullPath(omnidirectionalDepthGsRelPath));
	_omnidirectionalDepthShader->attachFragmentShaderFromFile(
		getAssetFullPath(omnidirectionalDepthFsRelPath));
	
	_omnidirectionalDepthShader->link();
}

void ShadowMapping::initDirectionalCascadeDepthShader() {
	_directionalCascadeDepthShader.reset(new GLSLProgram);

	_directionalCascadeDepthShader->attachVertexShaderFromFile(
		getAssetFullPath(directionalDepthCsmVsRelPath));
	_directionalCascadeDepthShader->attachGeometryShaderFromFile(
		getAssetFullPath(directionalDepthCsmGsRelPath));
	_directionalCascadeDepthShader->attachFragmentShaderFromFile(
		getAssetFullPath(directionalDepthCsmFsRelPath));

	_directionalCascadeDepthShader->link();
}

void ShadowMapping::initQuadShader() {
	_quadShader.reset(new GLSLProgram);
	_quadShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
	_quadShader->attachFragmentShaderFromFile(getAssetFullPath(quadFsRelPath));
	_quadShader->link();
}

void ShadowMapping::initQuadCascadeShader() {
	_quadCascadeShader.reset(new GLSLProgram);
	_quadCascadeShader->attachVertexShaderFromFile(getAssetFullPath(quadCsmVsRelPath));
	_quadCascadeShader->attachFragmentShaderFromFile(getAssetFullPath(quadCsmFsRelPath));
	_quadCascadeShader->link();
}

void ShadowMapping::initCubeShader() {
	_cubeShader.reset(new GLSLProgram);
	_cubeShader->attachVertexShaderFromFile(getAssetFullPath(cubeVsRelPath));
	_cubeShader->attachFragmentShaderFromFile(getAssetFullPath(cubeFsRelPath));
	_cubeShader->link();
}

void ShadowMapping::renderDirectionalLightShadowMap() {
	_framebuffer->bind();
	glViewport(0, 0, shadowMapResolution, shadowMapResolution);

	if (!_enableCascadeShadowMapping) {
		_framebuffer->attachTexture(*_depthTexture, GL_DEPTH_ATTACHMENT);
	} else {
		_framebuffer->attachTexture(*_depthTextureArray, GL_DEPTH_ATTACHMENT);
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	auto& shader = _enableCascadeShadowMapping ? _directionalCascadeDepthShader : _directionalDepthShader;
	shader->use();

	if (!_enableCascadeShadowMapping) {
		shader->setUniformMat4("lightSpaceMatrix", _directionalLightSpaceMatrix);
	} else {
		for (size_t i = 0; i < _directionalLightSpaceMatrices.size(); ++i) {
			shader->setUniformMat4(
				"lightSpaceMatrices[" + std::to_string(i) + "]", _directionalLightSpaceMatrices[i]);
		}
	}

	// 1. draw bunnies
	for (size_t i = 0; i < _bunnies.size(); ++i) {
		shader->setUniformMat4("model", _bunnies[i]->transform.getLocalMatrix());
		_bunnies[i]->draw();
	}

	// 2. draw ground
	shader->setUniformMat4("model", _ground->transform.getLocalMatrix());
	_ground->draw();

	_framebuffer->unbind();
}

void ShadowMapping::renderPointLightShadowMap() {
	_framebuffer->bind();
	_framebuffer->attachTexture(*_depthCubeTexture, GL_DEPTH_ATTACHMENT);
	
	glViewport(0, 0, shadowMapResolution, shadowMapResolution);
	glClear(GL_DEPTH_BUFFER_BIT);

	_omnidirectionalDepthShader->use();
	_omnidirectionalDepthShader->setUniformVec3("lightPosition", _pointLight->transform.position);
	_omnidirectionalDepthShader->setUniformFloat("zFar", _pointLightZfar);
	for (size_t i = 0; i < 6; ++i) {
		_omnidirectionalDepthShader->setUniformMat4(
			"lightSpaceMatrices[" + std::to_string(i) + "]", _pointLightSpaceMatrices[i]);
	}

	// 1. draw bunnies
	for (size_t i = 0; i < _bunnies.size(); ++i) {
		_omnidirectionalDepthShader->setUniformMat4("model", _bunnies[i]->transform.getLocalMatrix());
		_bunnies[i]->draw();
	}

	// 2. draw ground
	_omnidirectionalDepthShader->setUniformMat4("model", _ground->transform.getLocalMatrix());
	_ground->draw();

	_framebuffer->unbind();
}

void ShadowMapping::renderScene() {
	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, _windowWidth, _windowHeight);
	glCullFace(GL_BACK);

	_lambertShader->use();

	// camera info
	_lambertShader->setUniformMat4("projection", _camera->getProjectionMatrix());
	_lambertShader->setUniformMat4("view", _camera->getViewMatrix());
	_lambertShader->setUniformVec3("viewPosition", _camera->transform.position);

	// lights info
	_lambertShader->setUniformFloat("ambientLight.intensity", _ambientLight->intensity);
	_lambertShader->setUniformVec3("ambientLight.color", _ambientLight->color);

	_lambertShader->setUniformVec3("directionalLight.direction", _directionalLight->transform.getFront());
	_lambertShader->setUniformFloat("directionalLight.intensity", _directionalLight->intensity);
	_lambertShader->setUniformVec3("directionalLight.color", _directionalLight->color);
	_lambertShader->setUniformMat4("lightSpaceMatrix", _directionalLightSpaceMatrix);

	_lambertShader->setUniformVec3("pointLight.position", _pointLight->transform.position);
	_lambertShader->setUniformFloat("pointLight.intensity", _pointLight->intensity);
	_lambertShader->setUniformVec3("pointLight.color", _pointLight->color);
	_lambertShader->setUniformFloat("pointLight.kc", _pointLight->kc);
	_lambertShader->setUniformFloat("pointLight.kl", _pointLight->kl);
	_lambertShader->setUniformFloat("pointLight.kq", _pointLight->kq);
	_lambertShader->setUniformFloat("pointLightZfar", _pointLightZfar);

	// pcf
	_lambertShader->setUniformInt("directionalFilterRadius", _directionalFilterRadius);
	_lambertShader->setUniformBool("enableOmnidirectionalPCF", _enableOmnidirectionalPCF);

	// depth textures
	if (!_enableCascadeShadowMapping) {
		_lambertShader->setUniformInt("depthTexture", 0);
		_depthTexture->bind(0);
		_lambertShader->setUniformInt("cascadeCount", 0);
	} else {
		_lambertShader->setUniformInt("depthTextureArray", 2);
		_depthTextureArray->bind(2);
		_lambertShader->setUniformInt("cascadeCount", static_cast<int>(_directionalLightSpaceMatrices.size()));

		std::vector<float> distances = getCascadeDistances();
		for (size_t i = 1; i < distances.size(); ++i) {
			_lambertShader->setUniformFloat("cascadeZfars[" + std::to_string(i - 1) + "]", distances[i]);
		}

		for (size_t i = 0; i < _directionalLightSpaceMatrices.size(); ++i) {
			_lambertShader->setUniformMat4(
				"lightSpaceMatrices[" + std::to_string(i) + "]", _directionalLightSpaceMatrices[i]);
		}

		std::vector<float> biasModifiers = getCascadeBiasModifiers();
		for (size_t i = 0; i < biasModifiers.size(); ++i) {
			_lambertShader->setUniformFloat("cascadeBiasModifiers[" + std::to_string(i) + "]", biasModifiers[i]);
		}
	}

	_lambertShader->setUniformInt("depthCubeTexture", 1);
	_depthCubeTexture->bind(1);

	// 1. draw bunnies
	_lambertShader->setUniformVec3("material.ka", _bunnyMaterial->ka);
	_lambertShader->setUniformVec3("material.kd", _bunnyMaterial->kd);
	for (size_t i = 0; i < _bunnies.size(); ++i) {
		_lambertShader->setUniformMat4("model", _bunnies[i]->transform.getLocalMatrix());
		_bunnies[i]->draw();
	}

	// 2. draw ground
	_lambertShader->setUniformMat4("model", _ground->transform.getLocalMatrix());
	_lambertShader->setUniformVec3("material.ka", _groundMaterial->ka);
	_lambertShader->setUniformVec3("material.kd", _groundMaterial->kd);

	_ground->draw();

	// 3. draw lights
	_lightShader->use();
	_lightShader->setUniformMat4("projection", _camera->getProjectionMatrix());
	_lightShader->setUniformMat4("view", _camera->getViewMatrix());

	_lightShader->setUniformMat4("model", _directionalLight->transform.getLocalMatrix());
	_arrow->draw();

	_lightShader->setUniformMat4("model", _pointLight->transform.getLocalMatrix());
	_sphere->draw();
}

void ShadowMapping::renderDebugView() {
	switch (_debugView) {
		case DebugView::DirectionalLightDepthTexture:
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			_quadShader->use();
			_quadShader->setUniformInt("depthTexture", 0);
			_depthTexture->bind(0);

			_quad->draw();
			break;
		case DebugView::PointLightDepthTexture:
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			_cubeShader->use();
			_cubeShader->setUniformInt("depthCubeTexture", 0);
			_cubeShader->setUniformMat4("projection", _camera->getProjectionMatrix());
			_cubeShader->setUniformMat4("view", _camera->getViewMatrix());
			_cubeShader->setUniformMat4("model", _cube->transform.getLocalMatrix());
			_depthCubeTexture->bind(0);

			_cube->draw();
			break;
		case DebugView::CascadeDepthTextureLevel0:
		case DebugView::CascadeDepthTextureLevel1:
		case DebugView::CascadeDepthTextureLevel2:
		case DebugView::CascadeDepthTextureLevel3:
		case DebugView::CascadeDepthTextureLevel4:
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			_quadCascadeShader->use();
			_quadCascadeShader->setUniformInt("depthTextureArray", 0);
			_depthTextureArray->bind(0);
			_quadCascadeShader->setUniformInt("level", 
				static_cast<int>(_debugView) - static_cast<int>(DebugView::CascadeDepthTextureLevel0));

			_quad->draw();
			break;
		default:
			break;
	}
}

void ShadowMapping::renderUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Control Panel", nullptr, flags)) {
		ImGui::End();
	} else {
		ImGui::Text("directional light");
		ImGui::Separator();
		ImGui::SliderFloat("intensity##1", &_directionalLight->intensity, 0.0f, 1.0f);
		ImGui::SliderInt("pcf radius", &_directionalFilterRadius, 0, 3);
		ImGui::Checkbox("enable csm", &_enableCascadeShadowMapping);

		ImGui::Text("point light");
		ImGui::Separator();
		ImGui::SliderFloat("intensity##2", &_pointLight->intensity, 0.0f, 1.0f);
		ImGui::Checkbox("enable pcf", &_enableOmnidirectionalPCF);

		ImGui::Text("view shadow map");
		ImGui::Separator();

		static const char* debugViewItems[] = {
			"None",
			"directional",
			"omnidirectional",
			"cascade level 0",
			"cascade level 1",
			"cascade level 2",
			"cascade level 3",
			"cascade level 4",
		};

		ImGui::Combo("##2", (int*)(&_debugView), debugViewItems, IM_ARRAYSIZE(debugViewItems));

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ShadowMapping::updateDirectionalLightSpaceMatrix() {
	_directionalLightSpaceMatrix =
		glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f) * // projection
		glm::lookAt(                                             // view
			_directionalLight->transform.position,
			glm::vec3(0.0f, 0.0f, 0.0f),
			Transform::getDefaultUp());
}

void ShadowMapping::updateDirectionalLightSpaceMatrices() {
	_directionalLightSpaceMatrices.clear();

	const BoundingBox box = getSceneBoundingBox();
	std::vector<float> distances = getCascadeDistances();

	for (size_t i = 1; i < distances.size(); ++i) {
		// TODO: change the code here to get light space matrices for CSM
		// --------------------------------------------------------------
		_directionalLightSpaceMatrices.push_back(glm::mat4(1.0f));
		// --------------------------------------------------------------
	}
}

void ShadowMapping::updatePointLightSpaceMatrices() {
	const glm::mat4 projection = glm::perspective(
		glm::radians(90.0f), 1.0f, 1.0f, _pointLightZfar);

	const glm::vec3& eye = _pointLight->transform.position;
	const glm::mat4 views[6] = {
		glm::lookAt(eye, eye + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(eye, eye + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(eye, eye + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(eye, eye + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(eye, eye + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(eye, eye + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	};

	for (size_t i = 0; i < 6; ++i) {
		_pointLightSpaceMatrices[i] = projection * views[i];
	}
}

BoundingBox ShadowMapping::getSceneBoundingBox() const {
	auto getModelBoundingBox = [](const Model& model) {
		BoundingBox result;

		BoundingBox box = model.getBoundingBox();
		const glm::mat3 modelMatrix = model.transform.getLocalMatrix();

		glm::vec3 point;
		for (size_t i = 0; i < 2; ++i) {
			point.x = i ? box.max.x : box.min.x;
			for (int j = 0; j < 2; ++j) {
				point.y = j ? box.max.y : box.min.y;
				for (int k = 0; k < 2; ++k) {
					point.z = k ? box.max.z : box.min.z;
					// change the point from model space to world space 
					point = modelMatrix * point;
					result.min = glm::min(point, result.min);
					result.max = glm::max(point, result.max);
				}
			}
		}

		return result;
	};

	BoundingBox result;
	for (size_t i = 0; i < _bunnies.size(); ++i) {
		result += getModelBoundingBox(*_bunnies[i]);
	}
	result += getModelBoundingBox(*_ground);

	return result;
}

std::vector<float> ShadowMapping::getCascadeDistances() const {
	return std::vector<float> {
		_camera->znear,
		_camera->zfar / 50.0f,
		_camera->zfar / 25.0f,
		_camera->zfar / 10.0f,
		_camera->zfar / 2.0f,
		_camera->zfar
	};
}

std::vector<float> ShadowMapping::getCascadeBiasModifiers() const {
	// TODO: Change the value here
	return std::vector<float> {
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f
	};
}