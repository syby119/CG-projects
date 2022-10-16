#pragma once

#include "../base/application.h"
#include "../base/model.h"
#include "../base/light.h"
#include "../base/glsl_program.h"
#include "../base/texture2d.h"
#include "../base/camera.h"
#include "../base/skybox.h"
#include "../base/framebuffer.h"
#include "../base/fullscreen_quad.h"

enum class RenderMode {
	AlphaTesting,
	AlphaBlending,
	DepthPeeling
};

struct TransparentMaterial {
	glm::vec3 albedo;
	float ka;
	glm::vec3 kd;
	float transparent;
};

class Transparency : public Application {
public:
	Transparency(const Options& options);

	~Transparency();
private:
	enum RenderMode _renderMode = RenderMode::AlphaTesting;

	std::unique_ptr<Model> _knot;

	std::unique_ptr<TransparentMaterial> _knotMaterial;

	std::unique_ptr<Texture2D> _transparentTexture;

	std::unique_ptr<DirectionalLight> _light;
	std::unique_ptr<PerspectiveCamera> _camera;

	std::unique_ptr<GLSLProgram> _alphaTestingShader;
	std::unique_ptr<GLSLProgram> _alphaBlendingShader;

	// depth peeling resources
	std::unique_ptr<FullscreenQuad> _fullscreenQuad;

	std::unique_ptr<Framebuffer> _colorBlendFbo;
	std::unique_ptr<Texture2D> _colorBlendTexture;
	std::unique_ptr<Framebuffer> _fbos[2];
	std::unique_ptr<Texture2D> _colorTextures[2];
	std::unique_ptr<Texture2D> _depthTextures[2];

	std::unique_ptr<GLSLProgram> _depthPeelingInitShader;
	std::unique_ptr<GLSLProgram> _depthPeelingShader;
	std::unique_ptr<GLSLProgram> _depthPeelingBlendShader;
	std::unique_ptr<GLSLProgram> _depthPeelingFinalShader;

	GLuint _queryId = 0;

	void initAlphaTestingShader();

	void initAlphaBlendingShader();

	void initDepthPeelingShaders();

	void initDepthPeelingResources();

	void handleInput() override;

	void renderFrame() override;

	void renderWithAlphaTesting();

	void renderWithAlphaBlending();

	void renderWithDepthPeeling();

	void renderUI();
};