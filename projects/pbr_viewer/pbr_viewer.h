#pragma once 

#include <memory>
#include <vector>
#include <map>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/light.h"
#include "../base/glsl_program.h"
#include "../base/texture.h"
#include "../base/glsl_program.h"
#include "../base/uniform_buffer.h"
#include "../base/fullscreen_quad.h"

#include "./model.h"
#include "./skybox.h"
#include "./render_object.h"
#include "./camera_controller.h"

class PbrViewer: public Application {
public:
	PbrViewer(const Options& options);
	
	~PbrViewer();

private:
	std::unique_ptr<Model> _model;
	std::unique_ptr<GLSLProgram> _pbrShader;

	std::unique_ptr<PerspectiveCamera> _camera;
	std::unique_ptr<CameraController> _cameraController;
	
	std::unique_ptr<DirectionalLight> _directionalLight;
	
	std::unique_ptr<Skybox> _skybox;
	std::unique_ptr<GLSLProgram> _skyboxShader;

	std::unique_ptr<FullscreenQuad> _quad;
	std::unique_ptr<GLSLProgram> _quadShader;

	std::unique_ptr<UniformBuffer> _uboCamera;
	std::unique_ptr<UniformBuffer> _uboLights;
	std::unique_ptr<UniformBuffer> _uboEnvironment;

	std::vector<RenderObject> _opaqueQueue;
	std::vector<RenderObject> _alphaQueue;
	std::vector<RenderObject> _transparentQueue;

	enum class DebugInput : int{
		All = 0,
		Albedo,
		Roughness,
		Metallic,
		Normal,
		Occlusion,
		Emissive,
	};
	enum DebugInput _debugInput = { DebugInput::All };

	enum class SkyboxRenderMode : int {
		Raw = 0,
		Irradiance,
		Prefilter,
		BrdfLut
	};
	enum SkyboxRenderMode _skyboxRenderMode = { SkyboxRenderMode::Raw };

private:
	void handleInput() override;

	void renderFrame() override;

	void updateUniforms();

	void enqueueRenderables();

	void enqueueRenderable(const Node& node, glm::mat4 parentGlobalMatrix);

	void drawPrimitive(const Primitive& primitive) const;

	void clearScreen();

	void renderOpaqueQueue() const;

	void renderAlphaQueue() const;

	void renderTransparentQueue() const;

	void renderSkybox() const;

	void renderIrradianceMap() const;

	void renderPrefilterMap() const;
	
	void renderBrdfLutMap() const;

	void renderUI() const;

	void initShaders();

	void setPbrShaderUniforms(const RenderObject& object) const;

	void setupUniformBufferObjects();

	void confirmBindingPoints();

	void printRenderQueue(
		const std::string& name,
		const std::vector<RenderObject>& renderQueue) const;
};