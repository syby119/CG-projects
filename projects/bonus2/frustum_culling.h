#pragma once

#include <memory>
#include <glad/glad.h>

#include "../base/application.h"
#include "../base/model.h"
#include "../base/instanced_model.h"
#include "../base/glsl_program.h"
#include "../base/camera.h"
#include "../base/light.h"
#include "../base/texture2d.h"

struct DrawElementsIndirectCommand {
	unsigned int count;
	unsigned int instanceCount;
	unsigned int firstIndex;
	int  baseVertex;
	unsigned int baseInstance;
};

enum class Method {
	CPU, GPU
};

struct LineMaterial {
	glm::vec3 color;
	float width;
};

struct LambertMaterial {
	glm::vec3 kd;
	std::shared_ptr<Texture2D> mapKd;
};

class FrustumCulling : public Application {
public:
	FrustumCulling(const Options& options);

	~FrustumCulling();
private:
	std::unique_ptr<Model> _planet;

	std::unique_ptr<Model> _asternoid;
	std::vector<glm::mat4> _modelMatrices;
	int _amount = 10000;
	int _drawAsternoidCount = 0;

	std::unique_ptr<InstancedModel> _instancedAsternoids;

	std::unique_ptr<LineMaterial> _lineMaterial;
	std::unique_ptr<LambertMaterial> _planetMaterial;
	std::unique_ptr<LambertMaterial> _asternoidMaterial;

	std::unique_ptr<GLSLProgram> _lineShader;
	std::unique_ptr<GLSLProgram> _lineInstancedShader;
	std::unique_ptr<GLSLProgram> _lambertShader;
	std::unique_ptr<GLSLProgram> _lambertInstancedShader;

	std::unique_ptr<PerspectiveCamera> _camera;
	const float _cameraMoveSpeed = 10.0f;
	const float _cameraRotateSpeed = 0.05f;

	std::unique_ptr<DirectionalLight> _light;

	std::vector<int> _visibles;

	bool _showBoundingBox = false;

	enum Method _method = Method::CPU;

	// GPU frustum resources
	GLenum _transformFeedback = {};
	GLenum _transformFeedbackResultBuffer = {};
	std::unique_ptr<GLSLProgram> _frustumCullingShader;

	// indirect draw resources
	bool _indirectDrawEnabled = false;
	std::vector<DrawElementsIndirectCommand> _indirectDrawCmds;
	GLuint _indirectBuffer = {};

	void initModelMatrices();

	void initShaders();

	void initGPUCullingResources();

	void renderAsternoids();

	void renderAsternoidsIndirect();

	void handleInput() override;

	void renderFrame() override;
};