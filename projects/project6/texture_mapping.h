#pragma once

#include <memory>
#include <string>

#include "../base/application.h"
#include "../base/model.h"
#include "../base/light.h"
#include "../base/glsl_program.h"
#include "../base/texture2d.h"
#include "../base/camera.h"
#include "../base/skybox.h"

enum class RenderMode {
	Simple, Blend, Checker
};

struct SimpleMaterial {
	std::shared_ptr<Texture2D> mapKd;
};

struct BlendMaterial {
	glm::vec3 kds[2];
	std::shared_ptr<Texture2D> mapKds[2];
	float blend;
};

struct CheckerMaterial {
	int repeat;
	glm::vec3 colors[2];
};

class TextureMapping : public Application {
public:
	TextureMapping(const Options& options);
	
	~TextureMapping();

private:
	std::unique_ptr<Model> _sphere;

	std::unique_ptr<SimpleMaterial> _simpleMaterial;
	std::unique_ptr<BlendMaterial> _blendMaterial;
	std::unique_ptr<CheckerMaterial> _checkerMaterial;

	std::unique_ptr<PerspectiveCamera> _camera;
	std::unique_ptr<DirectionalLight> _light;

	std::unique_ptr<GLSLProgram> _simpleShader;
	std::unique_ptr<GLSLProgram> _blendShader;
	std::unique_ptr<GLSLProgram> _checkerShader;

	std::unique_ptr<SkyBox> _skybox;

	enum RenderMode _renderMode = RenderMode::Simple;

	void initSimpleShader();

	void initBlendShader();

	void initCheckerShader();

	void handleInput() override;

	void renderFrame() override;
};