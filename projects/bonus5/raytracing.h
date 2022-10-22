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

	~RayTracing();

private:
	std::unique_ptr<Model> _lucy;
	
	std::vector<Sphere> _balls;
	std::vector<Material> _ballMaterials;

	std::unique_ptr<TextureCubemap> _skybox;

	std::unique_ptr<FullscreenQuad> _screenQuad;

	std::unique_ptr<Camera> _camera;

	std::unique_ptr<GLSLProgram> _raytracingShader;
	std::unique_ptr<GLSLProgram> _drawScreenShader;

	std::unique_ptr<Framebuffer> _sampleFramebuffers[2];
	uint32_t _currentReadBufferID = 0;
	uint32_t _currentWriteBufferID = 1;
	uint32_t _sampleCount = 0;

	std::unique_ptr<Texture2D> _outFrames[2];
	std::unique_ptr<Texture2D> _rngStates[2];

	std::unique_ptr<Texture2D> _vertexBuffer;
	std::unique_ptr<Texture2D> _indexBuffer;

	std::unique_ptr<Texture2D> _sphereBuffer;
	std::unique_ptr<Texture2D> _primitiveBuffer;

	std::unique_ptr<Texture2D> _materialBuffer;
	std::unique_ptr<Texture2D> _bvhBuffer;

	bool _hasSphere = false;
	bool _useBVH = false;

	int _renderSceneIndex = 0;

	void handleInput() override;

	void renderFrame() override;

	void initShaders();

	void createBalls();

	void createRenderScene(int index);

	void createScene1();

	void createScene2();
	
	void createScene3();

	void createPrimitiveBuffer(const std::vector<Sphere>& spheres,
		const std::vector<Model*> models,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Material>& sphereMaterials,
		const std::vector<Material>& triangleMaterials);

	int getBufferHeight(size_t nObjects, size_t objectSize, size_t texComponent) const;

	static int toFloatLayout(int v);

	static size_t roundUp(size_t val, size_t number);
};