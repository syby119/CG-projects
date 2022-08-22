#pragma once

#include <memory>
#include <string>

#include <imgui.h>

#include "../base/application.h"
#include "../base/glsl_program.h"
#include "../base/model.h"
#include "../base/light.h"
#include "../base/camera.h"

enum class RenderMode {
	Ambient, Lambert, Phong
};

// I = ka * Ia
struct AmbientMaterial {
	glm::vec3 ka;
};

// I = sum { kd * Light[i] * cos(theta[i]) }
struct LambertMaterial {
	glm::vec3 kd;
};

// I = ka * Ia + sum { (kd * cos(theta[i]) + ks * cos(theta[i])^ns ) * Light[i] }
struct PhongMaterial {
	glm::vec3 ka;
	glm::vec3 kd;
	glm::vec3 ks;
	float ns;
};

class ShadingTutorial : public Application {
public:
	ShadingTutorial(const Options& options);

	~ShadingTutorial();
private:
	// model
	std::unique_ptr<Model> _bunny;

	// materials
	std::unique_ptr<AmbientMaterial> _ambientMaterial;
	std::unique_ptr<LambertMaterial> _lambertMaterial;
	std::unique_ptr<PhongMaterial> _phongMaterial;

	// shaders
	std::unique_ptr<GLSLProgram> _ambientShader;
	std::unique_ptr<GLSLProgram> _lambertShader;
	std::unique_ptr<GLSLProgram> _phongShader;

	// lights
	std::unique_ptr<AmbientLight> _ambientLight;
	std::unique_ptr<DirectionalLight> _directionalLight;
	std::unique_ptr<SpotLight> _spotLight;

	// camera
	std::unique_ptr<PerspectiveCamera> _camera;

	RenderMode _renderMode = RenderMode::Ambient;

	// I = ka * albedo
	void initAmbientShader();

	// I = ka * albedo + kd * max(cos<I, n>, 0)
	void initLambertShader();

	// I = ka * albedo + kd * cos<I, n> + ks * (max(cos<R, V>, 0) ^ ns)
	void initPhongShader();

	void handleInput() override;

	void renderFrame() override;
};