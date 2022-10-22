#pragma once

#include <vector>
#include <memory>

#include "../base/application.h"
#include "../base/glsl_program.h"
#include "../base/model.h"
#include "../base/texture2d.h"
#include "../base/skybox.h"
#include "../base/fullscreen_quad.h"
#include "../base/framebuffer.h"
#include "../base/camera.h"

#include "primitive.h"
#include "bvh.h"

class RayTracing : public Application {
public:
	RayTracing(const Options& options);

	~RayTracing() = default;

private:
	std::unique_ptr<Model> lucy;
	
	std::vector<Sphere> balls;
	std::vector<Material> ballMaterials;

	std::unique_ptr<TextureCubemap> skyBox;

	std::unique_ptr<FullscreenQuad> _screenQuad;

	std::unique_ptr<Camera> _camera;

	std::unique_ptr<GLSLProgram> raytracingProgram;
	std::unique_ptr<GLSLProgram> drawScreenProgram;

	std::unique_ptr<Framebuffer> _sampleFramebuffers[2];
	uint32_t currentReadBufferID = 0;
	uint32_t currentWriteBufferID = 1;
	uint32_t sampleCount = 0;

	std::unique_ptr<Texture2D> _outFrames[2];
	std::unique_ptr<Texture2D> _rngStates[2];

	std::unique_ptr<Texture2D> _vertexBuffer;
	std::unique_ptr<Texture2D> _indexBuffer;

	std::unique_ptr<Texture2D> _sphereBuffer;
	std::unique_ptr<Texture2D> _primitiveBuffer;

	std::unique_ptr<Texture2D> _materialBuffer;
	std::unique_ptr<Texture2D> _bvhBuffer;

	bool hasSphere = false;
	bool useBVH = true;

	void handleInput() override;

	void renderFrame() override;

	void initShaders();

	void createBalls();
	
	void createPrimitiveBuffer(const std::vector<Sphere>& spheres, 
	                           const std::vector<Model*> models, 
	                           const std::vector<glm::mat4>& transforms,
							   const std::vector<Material>& sphereMaterials, 
							   const std::vector<Material>& triangleMaterials);

	void createScene1();
	void createScene2();
	void createScene3();

	int getBufferHeight(size_t nObjects, size_t objectSize, size_t texComponent) const;

	static int toFloatLayout(int v);

	static size_t roundUp(size_t val, size_t number);
};