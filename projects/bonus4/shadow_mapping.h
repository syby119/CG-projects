#pragma once

#include <array>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/framebuffer.h"
#include "../base/texture2d.h"
#include "../base/texture_cubemap.h"
#include "../base/fullscreen_quad.h"

struct LambertMaterial {
	glm::vec3 ka = glm::vec3(0.05f);
	glm::vec3 kd = glm::vec3(0.8f);
};

enum class DebugView {
	None,
	DirectionalLightDepthTexture,
	PointLightDepthTexture,
	CascadeDepthTextureLevel0,
	CascadeDepthTextureLevel1,
	CascadeDepthTextureLevel2,
	CascadeDepthTextureLevel3,
	CascadeDepthTextureLevel4,
};

class ShadowMapping: public Application {
public:
	ShadowMapping(const Options& options);

	~ShadowMapping();

private:
	std::vector<std::unique_ptr<Model>> _bunnies;
	std::unique_ptr<LambertMaterial> _bunnyMaterial;

	std::unique_ptr<Model> _arrow;
	std::unique_ptr<Model> _sphere;
	std::unique_ptr<GLSLProgram> _lightShader;

	std::unique_ptr<Model> _ground;
	std::unique_ptr<LambertMaterial> _groundMaterial;

	std::unique_ptr<PerspectiveCamera> _camera;

	std::unique_ptr<AmbientLight> _ambientLight;

	std::unique_ptr<DirectionalLight> _directionalLight;
	std::unique_ptr<GLSLProgram> _directionalDepthShader;
	std::unique_ptr<GLSLProgram> _directionalCascadeDepthShader;

	std::unique_ptr<PointLight> _pointLight;
	std::unique_ptr<GLSLProgram> _omnidirectionalDepthShader;

	std::unique_ptr<GLSLProgram> _lambertShader;

	std::unique_ptr<Framebuffer> _framebuffer;
	std::unique_ptr<Texture2D> _depthTexture;
	std::unique_ptr<TextureCubemap> _depthCubeTexture;
	std::unique_ptr<Texture2DArray> _depthTextureArray;

	std::unique_ptr<FullscreenQuad> _quad;
	std::unique_ptr<GLSLProgram> _quadShader;
	std::unique_ptr<GLSLProgram> _quadCascadeShader;

	std::unique_ptr<Model> _cube;
	std::unique_ptr<GLSLProgram> _cubeShader;

	glm::mat4 _directionalLightSpaceMatrix;
	std::vector<glm::mat4> _directionalLightSpaceMatrices;
	glm::mat4 _pointLightSpaceMatrices[6];
	float _pointLightZfar = 100.0f;

	int _directionalFilterRadius = 0;
	bool _enableOmnidirectionalPCF = false;
	bool _enableCascadeShadowMapping = false;

	DebugView _debugView = DebugView::None;

	void handleInput() override;

	void renderFrame() override;

	void initGround();

	void initShaders();

	void initDirectionalDepthShader();

	void initOmnidirectionalDepthShader();

	void initDirectionalCascadeDepthShader();

	void initLambertShader();

	void initLightShader();

	void initQuadShader();

	void initQuadCascadeShader();

	void initCubeShader();

	void renderDirectionalLightShadowMap();

	void renderPointLightShadowMap();

	void renderScene();

	void renderDebugView();

	void renderUI();

	void updateDirectionalLightSpaceMatrix();

	void updateDirectionalLightSpaceMatrices();

	void updatePointLightSpaceMatrices();

	BoundingBox getSceneBoundingBox() const;

	std::vector<float> getCascadeDistances() const;

	std::vector<float> getCascadeBiasModifiers() const;
};