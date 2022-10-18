#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "debug_print.h"
#include "pbr_viewer.h"

const std::string modelRelPath = "gltf/DamagedHelmet.gltf";
//const std::string modelRelPath = "gltf/drone/scene.gltf";
//const std::string modelRelPath = "gltf/grey_knight/scene.gltf";

const std::string pbrVertShaderRelPath = "shader/pbr_viewer/pbr.vert";
const std::string pbrFragShaderRelPath = "shader/pbr_viewer/pbr.frag";

const std::string skyboxVertShaderRelPath = "shader/pbr_viewer/skybox.vert";
const std::string skyboxFragShaderRelPath = "shader/pbr_viewer/skybox.frag";

const std::string equirectVertShaderRelPath = "shader/pbr_viewer/filter_cube.vert";
const std::string equirectFragShaderRelPath = "shader/pbr_viewer/equirectangular_to_cubemap.frag";

const std::string irradianceVertShaderRelPath = "shader/pbr_viewer/filter_cube.vert";
const std::string irradianceFragShaderRelPath = "shader/pbr_viewer/irradiance.frag";

const std::string prefilterVertShaderRelPath = "shader/pbr_viewer/filter_cube.vert";
const std::string prefilterFragShaderRelPath = "shader/pbr_viewer/prefilter.frag";

const std::string brdfLutVertShaderRelPath = "shader/pbr_viewer/brdf_lut.vert";
const std::string brdfLutFragShaderRelPath = "shader/pbr_viewer/brdf_lut.frag";

const std::string quadVertShaderRelPath = "shader/pbr_viewer/quad.vert";
const std::string quadFragShaderRelPath = "shader/pbr_viewer/quad.frag";

const std::string skyboxTextureRelPaths = "texture/hdr/newport_loft.hdr";

PbrViewer::PbrViewer(const Options& options): Application(options) {
	_model.reset(new Model(getAssetFullPath(modelRelPath)));

	// camera
	_camera.reset(new PerspectiveCamera(
		glm::radians(45.0f),
		1.0f * _windowWidth / _windowHeight,
		0.1f, 10000.0f));
	_camera->transform.position = { 0.0f, 0.0f, 5.0f };
	// camera controller
	_cameraController.reset(new CameraController(*_camera, glm::vec3(0.0f), _windowWidth, _windowHeight));

	// lights
	_directionalLight.reset(new DirectionalLight());
	_directionalLight->transform.rotation = glm::quatLookAt({
		sin(glm::radians(75.0f))* cos(glm::radians(45.0f)),
		sin(glm::radians(45.0f)),
		cos(glm::radians(75.0f))* cos(glm::radians(45.0f))}, 
		Transform::getDefaultUp());
	_directionalLight->intensity = 10.0f;

	// skybox
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	_skybox.reset(new Skybox(
		getAssetFullPath(skyboxTextureRelPaths), 
		getAssetFullPath(equirectVertShaderRelPath),
		getAssetFullPath(equirectFragShaderRelPath), 
		512));
	
	_skybox->generateIrradianceMap(
		getAssetFullPath(irradianceVertShaderRelPath),
		getAssetFullPath(irradianceFragShaderRelPath), 
		32, 
		glm::radians(1.0f), 
		glm::radians(1.0f));

	_skybox->generatePrefilterMap(
		getAssetFullPath(prefilterVertShaderRelPath),
		getAssetFullPath(prefilterFragShaderRelPath),
		128, 
		4096);

	_skybox->generateBrdfLutMap(
		getAssetFullPath(brdfLutVertShaderRelPath),
		getAssetFullPath(brdfLutFragShaderRelPath),
		512,
		4096);

	// fullscreen quad
	_quad.reset(new FullscreenQuad);

	initShaders();

	setupUniformBufferObjects();
	
	confirmBindingPoints();

	// imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();
}

PbrViewer::~PbrViewer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void PbrViewer::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return ;
	}

	if (!ImGui::GetIO().WantCaptureMouse) {
		_cameraController->update(_input, _deltaTime);
	}

	_input.forwardState();

	updateUniforms();
	enqueueRenderables();

	//printRenderQueue("Opaque Queue", _opaqueQueue);
	//printRenderQueue("Alpha Queue", _alphaQueue);
	//printRenderQueue("Transparent Queue", _transparentQueue);
}

void PbrViewer::enqueueRenderables() {
	_opaqueQueue.clear();
	_alphaQueue.clear();
	_transparentQueue.clear();

	static glm::mat4 globalMatrix = glm::mat4(1.0f);
	//globalMatrix = glm::rotate(globalMatrix, _deltaTime * 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	for (const Node* node : _model->getRootNodes()) {
		enqueueRenderable(*node, globalMatrix);
	}
}

void PbrViewer::enqueueRenderable(const Node& node, glm::mat4 parentGlobalMatrix) {
	glm::mat4 nodeGlobalMatrix = parentGlobalMatrix * node.transform.getLocalMatrix();
	
	for (const auto& primitive : node.primitives) {
		RenderObject object = { nodeGlobalMatrix, &primitive };
		switch (primitive.material->alphaMode) {
			case Material::AlphaMode::Opaque:
				_opaqueQueue.emplace_back(object);
				break;
			case Material::AlphaMode::Mask:
				_alphaQueue.emplace_back(object);
				break;
			case Material::AlphaMode::Blend:
				_transparentQueue.emplace_back(object);
				break;
		}
	}

	for (const Node* childNode : node.children) {
		enqueueRenderable(*childNode, nodeGlobalMatrix);
	}
}

void PbrViewer::drawPrimitive(const Primitive& primitive) const {
	// double sided
	bool doubleSided = primitive.material->doubleSided;
	if (!doubleSided) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	// draw call
	glBindVertexArray(primitive.vertexArray);
	if (primitive.indexCount > 0) {
		glDrawElements(
			GL_TRIANGLES,
			primitive.indexCount,
			GL_UNSIGNED_INT,
			(GLvoid*)(sizeof(uint32_t) * primitive.firstIndex));
	} else {
		glDrawArrays(
			GL_TRIANGLES,
			primitive.firstVertex,
			primitive.vertexCount);
	}
	glBindVertexArray(0);

	// restore opengl state
	if (!doubleSided) {
		glDisable(GL_CULL_FACE);
	}
}

void PbrViewer::renderFrame() {
	showFpsInWindowTitle();
	clearScreen();

	renderOpaqueQueue();

	renderAlphaQueue();

	renderSkybox();

	renderTransparentQueue();

	renderUI();
}

void PbrViewer::clearScreen() {
	glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
}

void PbrViewer::updateUniforms() {
	// camera
	_uboCamera->update("projection", _camera->getProjectionMatrix());
	_uboCamera->update("view", _camera->getViewMatrix());
	_uboCamera->update("viewPosition", _camera->transform.position);

	// lights
	_uboLights->update("directionalLightCount", 1);
	_uboLights->update("directionalLights[0].direction", _directionalLight->transform.getFront());
	_uboLights->update("directionalLights[0].color", _directionalLight->color);
	_uboLights->update("directionalLights[0].intensity", _directionalLight->intensity);
	_uboLights->update("pointLightCount", 0);
	_uboLights->update("spotLightCount", 0);

	// IBL
	_uboEnvironment->update("exposure", _skybox->exposure);
	_uboEnvironment->update("gamma", _skybox->gamma);
	_uboEnvironment->update("maxPrefilterMipLevel", _skybox->getMaxPrefilterMipLevel());
	_uboEnvironment->update("scaleIBLAmbient", _skybox->scaleIBLAmbient);
}

void PbrViewer::renderOpaqueQueue() const {
	for (const auto& object : _opaqueQueue) {
		_pbrShader->use();
		setPbrShaderUniforms(object);
		drawPrimitive(*object.primitive);
	}
}

void PbrViewer::renderAlphaQueue() const {
	for (const auto& object : _alphaQueue) {
		_pbrShader->use();
		setPbrShaderUniforms(object);
		drawPrimitive(*object.primitive);
	}
}

void PbrViewer::renderTransparentQueue() const {
	// TODO: sort the object from near to far
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for (const auto& object : _transparentQueue) {
		_pbrShader->use();
		setPbrShaderUniforms(object);
		drawPrimitive(*object.primitive);
	}
	glDisable(GL_BLEND);
}

void PbrViewer::renderSkybox() const {
	switch (_skyboxRenderMode) {
		case SkyboxRenderMode::Irradiance:
			renderIrradianceMap();
			break;
		case SkyboxRenderMode::Prefilter:
			renderPrefilterMap();
			break;
		case SkyboxRenderMode::BrdfLut:
			renderBrdfLutMap();
			break;
		default:
			glDepthFunc(GL_LEQUAL);
			_skyboxShader->use();
			_skyboxShader->setUniformInt("environmentMap", 0);
			_skyboxShader->setUniformFloat("lod", _skybox->backgroundLod);
			_skybox->bindEnvironmentMap(0);
			glBindSampler(0, 0);
			_skybox->draw();
			glDepthFunc(GL_LESS);
			break;
	}
}

void PbrViewer::renderIrradianceMap() const {
	glDepthFunc(GL_LEQUAL);
	_skyboxShader->use();
	_skyboxShader->setUniformInt("environmentMap", 0);
	_skyboxShader->setUniformFloat("lod", 0.0f);
	_skybox->irradianceMap->bind(0);
	glBindSampler(0, 0);
	_skybox->draw();
	glDepthFunc(GL_LESS);
}

void PbrViewer::renderPrefilterMap() const {
	glDepthFunc(GL_LEQUAL);
	_skyboxShader->use();
	_skyboxShader->setUniformInt("environmentMap", 0);
	_skyboxShader->setUniformFloat("environmentMap", _skybox->backgroundLod);
	_skybox->prefilterMap->bind(0);
	glBindSampler(0, 0);
	_skybox->draw();
	glDepthFunc(GL_LESS);
}

void PbrViewer::renderBrdfLutMap() const {
	_quadShader->use();
	_quadShader->setUniformInt("inputTexture", 0);
	_skybox->brdfLutMap->bind(0);
	glBindSampler(0, 0);
	_quad->draw();
}

void PbrViewer::renderUI() const {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Control Panel", nullptr, flags)) {
		ImGui::End();
	} else {
		if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("directional light");
			ImGui::Separator();
			ImGui::SliderFloat("intensity", &_directionalLight->intensity, 0.0f, 20.0f);
			ImGui::ColorEdit3("color", (float*)&_directionalLight->color);
			ImGui::Text("image based lighting");
			ImGui::Separator();
			ImGui::SliderFloat("blur", &_skybox->backgroundLod, 0.0f, 8.0f);
			//ImGui::SliderFloat("exposure", &_skybox->exposure, 0.0f, 10.0f);
			//ImGui::SliderFloat("gamma", &_skybox->gamma, 0.1f, 4.0f);
			ImGui::SliderFloat("scale", &_skybox->scaleIBLAmbient, 0.0f, 1.5f);
		}

		if (ImGui::CollapsingHeader("Debug View", ImGuiTreeNodeFlags_DefaultOpen)) {
			static const char* pbrChannels[] = {
				"All", "Albedo", "Roughness", "Metallic", "Normal", "Occlusion", "Emissive"
			};

			ImGui::Combo("pbr channel", (int*)(&_debugInput), pbrChannels, IM_ARRAYSIZE(pbrChannels));

			static const char* skyboxTextureItems[] = {
				"Raw", "Irradiance", "Prefilter", "BrdfLut"
			};

			ImGui::Combo("skybox texture", (int*)(& _skyboxRenderMode), 
				skyboxTextureItems, IM_ARRAYSIZE(skyboxTextureItems));
		}
		
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void PbrViewer::initShaders() {
	_pbrShader.reset(new GLSLProgram);
	_pbrShader->attachVertexShaderFromFile(getAssetFullPath(pbrVertShaderRelPath));
	_pbrShader->attachFragmentShaderFromFile(getAssetFullPath(pbrFragShaderRelPath));
	_pbrShader->link();

	_skyboxShader.reset(new GLSLProgram);
	_skyboxShader->attachVertexShaderFromFile(getAssetFullPath(skyboxVertShaderRelPath));
	_skyboxShader->attachFragmentShaderFromFile(getAssetFullPath(skyboxFragShaderRelPath));
	_skyboxShader->link();

	_quadShader.reset(new GLSLProgram);
	_quadShader->attachVertexShaderFromFile(getAssetFullPath(quadVertShaderRelPath));
	_quadShader->attachFragmentShaderFromFile(getAssetFullPath(quadFragShaderRelPath));
	_quadShader->link();
}

void PbrViewer::setPbrShaderUniforms(const RenderObject& object) const {
	const PbrMaterial* material = object.primitive->material;
	_pbrShader->setUniformMat4("model", object.globalMatrix);
	_pbrShader->setUniformVec4("material.albedoFactor", material->albedoFactor);
	_pbrShader->setUniformVec4("material.emissiveFactor", material->emissiveFactor);
	_pbrShader->setUniformFloat("material.metallicFactor", material->metallicFactor);
	_pbrShader->setUniformFloat("material.roughnessFactor", material->roughnessFactor);
	_pbrShader->setUniformFloat("material.occlusionStrength", material->occlusionStrength);
	_pbrShader->setUniformInt("material.albedoTexCoordSet", material->texCoordSets.albedo);
	_pbrShader->setUniformInt("material.metallicTexCoordSet", material->texCoordSets.metallic);
	_pbrShader->setUniformInt("material.roughnessTexCoordSet", material->texCoordSets.roughness);
	_pbrShader->setUniformInt("material.normalTexCoordSet", material->texCoordSets.normal);
	_pbrShader->setUniformInt("material.emissiveTexCoordSet", material->texCoordSets.emissive);
	_pbrShader->setUniformInt("material.occlusionTexCoordSet", material->texCoordSets.occlusion);

	_pbrShader->setUniformBool("material.doubleSided", material->doubleSided);

	switch (material->alphaMode) {
	case Material::AlphaMode::Opaque:
		_pbrShader->setUniformBool("material.alphaMask", false); break;
	case Material::AlphaMode::Mask:
		_pbrShader->setUniformBool("material.alphaMask", true); break;
	case Material::AlphaMode::Blend:
		// try to discard to opacity that is near zero: 
		// when occur the blend matrial, the data parser has set alphaMask false,
		// but the alphaCutoff 0.05, which should have been ignored when alpha mode is blend
		// we can use the thick to discard the unwanted opacity texture
		_pbrShader->setUniformBool("material.alphaMask", true); break;
	}
	_pbrShader->setUniformFloat("material.alphaMaskCutoff", material->alphaCutoff);

	// textures
	if (material->albedoMap && material->texCoordSets.albedo >= 0) {
		_pbrShader->setUniformInt("albedoMap", 0);
		material->albedoMap->bind(0);
		if (material->albeodoSampler) {
			material->albeodoSampler->bind(0);
		}
	}

	if (material->roughnessMap && material->texCoordSets.roughness >= 0) {
		_pbrShader->setUniformInt("roughnessMap", 1);
		material->roughnessMap->bind(1);
		if (material->roughnessSampler) {
			material->roughnessSampler->bind(1);
		}
	}

	if (material->metallicMap && material->texCoordSets.metallic >= 0) {
		_pbrShader->setUniformInt("metallicMap", 2);
		material->metallicMap->bind(2);
		if (material->metallicSampler) {
			material->metallicSampler->bind(2);
		}
	}

	if (material->normalMap && material->texCoordSets.normal >= 0) {
		_pbrShader->setUniformInt("normalMap", 3);
		material->normalMap->bind(3);
		if (material->normalSampler) {
			material->normalSampler->bind(3);
		}
	}

	if (material->occlusionMap && material->texCoordSets.occlusion >= 0) {
		_pbrShader->setUniformInt("occlusionMap", 4);
		material->occlusionMap->bind(4);
		if (material->occlusionSampler) {
			material->occlusionSampler->bind(4);
		}
	}

	if (material->emissiveMap && material->texCoordSets.emissive >= 0) {
		_pbrShader->setUniformInt("emissiveMap", 5);
		material->emissiveMap->bind(5);
		if (material->emissiveSampler) {
			material->emissiveSampler->bind(5);
		}
	}

	// IBL textures
	_pbrShader->setUniformInt("irradianceMap", 6);
	_skybox->irradianceMap->bind(6);

	_pbrShader->setUniformInt("prefilterMap", 7);
	_skybox->prefilterMap->bind(7);

	_pbrShader->setUniformInt("brdfLutMap", 8);
	_skybox->brdfLutMap->bind(8);

	// debug
	_pbrShader->setUniformInt("debugInput", static_cast<int>(_debugInput));
}

void PbrViewer::setupUniformBufferObjects() {
	// uboCamera
	int uboCameraSize = _pbrShader->getUniformBlockSize("uboCamera");
	if (uboCameraSize <= 0) {
		throw std::runtime_error("get uboCamera size failure");
	}

	_uboCamera.reset(new UniformBuffer(uboCameraSize, GL_DYNAMIC_DRAW));

	std::string uboCameraVariableNames[] = {
		"projection",
		"view",
		"viewPosition",
	};

	for (const auto& name : uboCameraVariableNames) {
		int offset = _pbrShader->getUniformBlockVariableOffset(name);
		if (offset <= -1) {
			throw std::runtime_error("get uboCamera." + name + " offset failure");
		} else {
			_uboCamera->setOffset(name, static_cast<size_t>(offset));
		}
	}

	// uboLights
	int uboLightsSize = _pbrShader->getUniformBlockSize("uboLights");
	if (uboLightsSize <= 0) {
		throw std::runtime_error("get uboLights size failure");
	}

	_uboLights.reset(new UniformBuffer(uboLightsSize, GL_DYNAMIC_DRAW));

	std::vector<std::string> uboLightsVariableNames = {
		"directionalLightCount",
		"pointLightCount",
		"spotLightCount"
	};

	constexpr int maxDirectionalLights = 4;
	for (int i = 0; i < maxDirectionalLights; ++i) {
		std::string prefix = "directionalLights[" + std::to_string(i) + "]";
		uboLightsVariableNames.push_back(prefix + ".direction");
		uboLightsVariableNames.push_back(prefix + ".color");
		uboLightsVariableNames.push_back(prefix + ".intensity");
	}

	constexpr int maxPointLights = 8;
	for (int i = 0; i < maxPointLights; ++i) {
		std::string prefix = "pointLights[" + std::to_string(i) + "]";
		uboLightsVariableNames.push_back(prefix + ".position");
		uboLightsVariableNames.push_back(prefix + ".direction");
		uboLightsVariableNames.push_back(prefix + ".color");
		uboLightsVariableNames.push_back(prefix + ".intensity");
		uboLightsVariableNames.push_back(prefix + ".kc");
		uboLightsVariableNames.push_back(prefix + ".kl");
		uboLightsVariableNames.push_back(prefix + ".kq");
	}

	constexpr int maxSpotLights = 8;
	for (int i = 0; i < maxSpotLights; ++i) {
		std::string prefix = "spotLights[" + std::to_string(i) + "]";
		uboLightsVariableNames.push_back(prefix + ".position");
		uboLightsVariableNames.push_back(prefix + ".direction");
		uboLightsVariableNames.push_back(prefix + ".color");
		uboLightsVariableNames.push_back(prefix + ".intensity");
		uboLightsVariableNames.push_back(prefix + ".kc");
		uboLightsVariableNames.push_back(prefix + ".kl");
		uboLightsVariableNames.push_back(prefix + ".kq");
		uboLightsVariableNames.push_back(prefix + ".angle");
	}

	for (const auto& name : uboLightsVariableNames) {
		int offset = _pbrShader->getUniformBlockVariableOffset(name);
		if (offset <= -1) {
			throw std::runtime_error("get uboLights." + name + " offset failure");
		} else {
			_uboLights->setOffset(name, static_cast<size_t>(offset));
		}
	}

	// uboEnvironment
	int uboEnvironmentSize = _pbrShader->getUniformBlockSize("uboEnvironment");
	if (uboEnvironmentSize <= 0) {
		throw std::runtime_error("get uboEnvironment size failure");
	}
	_uboEnvironment.reset(new UniformBuffer(uboEnvironmentSize, GL_DYNAMIC_DRAW));

	std::string uboEnvironmentNames[] = {
		"exposure",
		"gamma",
		"maxPrefilterMipLevel",
		"scaleIBLAmbient"
	};

	for (const auto& name : uboEnvironmentNames) {
		int offset = _pbrShader->getUniformBlockVariableOffset(name);
		if (offset <= -1) {
			throw std::runtime_error("get uboEnvironment." + name + " offset failure");
		} else {
			_uboEnvironment->setOffset(name, static_cast<size_t>(offset));
		}
	}
}

void PbrViewer::confirmBindingPoints() {
	// ubo binding point
	_uboCamera->setBindingPoint(0);
	_uboLights->setBindingPoint(1);
	_uboEnvironment->setBindingPoint(2);

	// pbr shader binding point
	_pbrShader->setUniformBlockBinding("uboCamera", 0);
	_pbrShader->setUniformBlockBinding("uboLights", 1);
	_pbrShader->setUniformBlockBinding("uboEnvironment", 2);

	// skybox shader binding point
	_skyboxShader->setUniformBlockBinding("uboCamera", 0);
}

void PbrViewer::printRenderQueue(
	const std::string& name, 
	const std::vector<RenderObject>& renderQueue) const {

	std::cout     << "+ " << name << "\n";
	for (size_t i = 0; i < renderQueue.size(); ++i) {
		const glm::mat4 globalMatrix = glm::transpose(renderQueue[i].globalMatrix);
		const Primitive* primitive = renderQueue[i].primitive;
		std::cout << "  + object[" << i << "]:" << "\n";
		std::cout << "    + globalMatrix(row): "                        << "\n";
		std::cout << "        " << globalMatrix[0] << "\n";
		std::cout << "        " << globalMatrix[1] << "\n";
		std::cout << "        " << globalMatrix[2] << "\n";
		std::cout << "        " << globalMatrix[3] << "\n";
		std::cout << "    + vertexArray: " << primitive->vertexArray    << "\n";
		std::cout << "    + firstVertex: " << primitive->firstVertex    << "\n";
		std::cout << "    + VertexCount: " << primitive->vertexCount    << "\n";
		std::cout << "    + firstIndex:  " << primitive->firstIndex     << "\n";
		std::cout << "    + indexCount:  " << primitive->indexCount     << "\n";
		std::cout << "    + material:    " << primitive->material->name << "\n";
	}
}
