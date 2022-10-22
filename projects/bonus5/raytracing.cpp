#include <iostream>
#include <string>
#include <unordered_map>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "../base/vertex.h"
#include "random.h"
#include "raytracing.h"

static constexpr int BufferWidth = 2048;

const std::string lucyRelPath = "obj/lucy.obj";

const std::string quadVsRelPath = "shader/bonus5/quad.vert";
const std::string quadFsRelPath = "shader/bonus5/quad.frag";

const std::string raytracingVsRelPath = "shader/bonus5/quad.vert";
const std::string raytracingFsRelPath = "shader/bonus5/raytracing.frag";

const std::vector<std::string> skyboxTextureRelPaths = {
	"texture/skyboxrt/right.jpg",
	"texture/skyboxrt/left.jpg",
	"texture/skyboxrt/top.jpg",
	"texture/skyboxrt/bottom.jpg",
	"texture/skyboxrt/front.jpg",
	"texture/skyboxrt/back.jpg",
};

RayTracing::RayTracing(const Options& options): Application(options) {
	_lucy.reset(new Model(getAssetFullPath(lucyRelPath)));

	std::vector<std::string> skyBoxTexturePaths;
	for (size_t i = 0; i < skyboxTextureRelPaths.size(); ++i) {
		skyBoxTexturePaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
	}
	_skybox.reset(new ImageTextureCubemap(skyBoxTexturePaths));

	_camera.reset(new PerspectiveCamera(
		glm::radians(60.0f), static_cast<float>(_windowWidth) / _windowHeight, 0.1f, 1000.0f));
	_camera->transform.position = glm::vec3(15.0f, 3.0f, 4.0f);
	_camera->transform.lookAt(glm::vec3(0.0f));

	createBalls();

	initShaders();

	_screenQuad.reset(new FullscreenQuad);

	// rngInitState
	const int pixelCount = _windowWidth * _windowHeight;
	std::vector<unsigned int> rngStateInitVals;
	rngStateInitVals.reserve(pixelCount);
	for (int i = 0; i < pixelCount; ++i) {
		rngStateInitVals.push_back(1664525 * i + 1013904223);
	}

	for (int i = 0; i < 2; ++i) {
		_sampleFramebuffers[i].reset(new Framebuffer);
		_sampleFramebuffers[i]->bind();
		_sampleFramebuffers[i]->drawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

		_outFrames[i].reset(new Texture2D(GL_RGBA32F, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));
		_outFrames[i]->bind();
		_outFrames[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		_outFrames[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		_outFrames[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		_outFrames[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		_sampleFramebuffers[i]->attachTexture(*_outFrames[i], GL_COLOR_ATTACHMENT0);

		_rngStates[i].reset(new Texture2D(
			GL_R32UI, _windowWidth, _windowHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, rngStateInitVals.data()));
		_rngStates[i]->bind();
		_rngStates[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		_rngStates[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		_rngStates[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		_rngStates[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		_sampleFramebuffers[i]->attachTexture(*_rngStates[i], GL_COLOR_ATTACHMENT1);

		_sampleFramebuffers[i]->unbind();
	}

	createRenderScene(_renderSceneIndex);

	// init imGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init();
}

RayTracing::~RayTracing() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void RayTracing::handleInput() {
	if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
		glfwSetWindowShouldClose(_window, true);
		return;
	}

	static int lastSceneIndex = _renderSceneIndex;
	if (lastSceneIndex != _renderSceneIndex) {
		createRenderScene(_renderSceneIndex);
		lastSceneIndex = _renderSceneIndex;
		_sampleCount = 0;
	}
}

void RayTracing::renderFrame() {
	showFpsInWindowTitle();

	glDisable(GL_DEPTH_TEST);

	glm::mat4 cameraToWorld = glm::inverse(_camera->getViewMatrix());
	glm::mat4 cameraToScreen = _camera->getProjectionMatrix();
	glm::mat4 screenToRaster = glm::scale(glm::mat4(1.0f), glm::vec3(float(_windowWidth) / 2.0f,
		float(_windowHeight) / 2.0f, 1.0f)) *
		glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));

	glm::mat4 rasterToScreen = glm::inverse(screenToRaster);
	glm::mat4 rasterToCamera = glm::inverse(cameraToScreen) * rasterToScreen;

	_sampleFramebuffers[_currentWriteBufferID]->bind();
	_raytracingShader->use();
	_raytracingShader->setUniformUint("totalSamples", _sampleCount);
	_raytracingShader->setUniformMat4("camera.cameraToWorld", cameraToWorld);
	_raytracingShader->setUniformMat4("camera.rasterToCamera", rasterToCamera);

	_raytracingShader->setUniformInt("sky", 0);
	_skybox->bind(0);
	
	_sphereBuffer->bind(1);
	_raytracingShader->setUniformInt("sphereBuffer", 1);

	_raytracingShader->setUniformInt("materialBuffer", 2);
	_materialBuffer->bind(2);

	_raytracingShader->setUniformInt("primitiveBuffer", 3);
	_primitiveBuffer->bind(3);

	_raytracingShader->setUniformInt("RTResult", 4);
	_outFrames[_currentReadBufferID]->bind(4);

	_raytracingShader->setUniformInt("oldRngState", 5);
	_rngStates[_currentReadBufferID]->bind(5);

	_indexBuffer->bind(6);
	_raytracingShader->setUniformInt("triangleIndexBuffer", 6);
	_vertexBuffer->bind(7);
	_raytracingShader->setUniformInt("vertexBuffer", 7);

	_bvhBuffer->bind(8);
	_raytracingShader->setUniformInt("bvh", 8);

	_screenQuad->draw();

	_sampleFramebuffers[_currentWriteBufferID]->unbind();
	
	// render the result to the screen
	_drawScreenShader->use();
	_drawScreenShader->setUniformInt("frame", 0);
	
	_outFrames[_currentWriteBufferID]->bind(0);
	_screenQuad->draw();

	// update
	++_sampleCount;
	std::swap(_currentReadBufferID, _currentWriteBufferID);

	// render UI
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Once, ImVec2(0.0f, 0.0f));

	const auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Control Panel", nullptr, flags)) {
		ImGui::End();
	} else {
		ImGui::Text("switch scenes");
		ImGui::Separator();
		static const char* scenes[] = {
			"scene 1", "scene 2", "scene 3"
		};

		ImGui::Combo("##1", &_renderSceneIndex, scenes, IM_ARRAYSIZE(scenes));

		ImGui::NewLine();

		ImGui::Text("statistics");
		ImGui::Separator();
		ImGui::Text("samples: %u", _sampleCount);

		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RayTracing::initShaders() {
	// TODO: modify raytracing.frag code to achieve raytracing
	_raytracingShader.reset(new GLSLProgram);
	_raytracingShader->attachVertexShaderFromFile(getAssetFullPath(raytracingVsRelPath));
	_raytracingShader->attachFragmentShaderFromFile(getAssetFullPath(raytracingFsRelPath));
	_raytracingShader->link();

	_drawScreenShader.reset(new GLSLProgram);
	_drawScreenShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
	_drawScreenShader->attachFragmentShaderFromFile(getAssetFullPath(quadFsRelPath));
	_drawScreenShader->link();
}

int RayTracing::getBufferHeight(size_t nObjects, size_t objectSize, size_t texComponent) const {
	size_t componentPerObject = objectSize / (sizeof(float) * texComponent);
	return static_cast<int>(nObjects * componentPerObject + BufferWidth - 1) / BufferWidth;
}

void RayTracing::createBalls() {
    _balls.push_back(Sphere(glm::vec3(0.0f, -1000.0f, 0.0f), 1000.0f));
	_ballMaterials.push_back(Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.5f, 0.5f, 0.5f)));
	for (int a = -12; a < 12; ++a) {
        for (int b = -12; b < 12; ++b) {
            auto chooseMat = randomFloat();
            glm::vec3 center(a + 0.9f * randomFloat(), 0.2f, b + 0.9f * randomFloat());
	
            if ((glm::length(center - glm::vec3(0.0f, 0.2f, 0.0f)) > 2.0f) &&
				(glm::length(center - glm::vec3(4.0f, 0.2f, -2.0f)) > 2.0f) &&
				(glm::length(center - glm::vec3(-4.0f, 0.2f, 2.0f)) > 2.0f) &&
				(glm::length(center - glm::vec3(4.0f, 0.0f, 5.0f)) > 1.0f)) {
                Material material;
                if (chooseMat < 0.8f) {
					material.type = Material::Type::Lambertian;
					material.ior = 1.0f;
					material.fuzz = 0.0f;
					material.albedo = randomVec3() * randomVec3();
                } else if (chooseMat < 0.95f) {
					material.type = Material::Type::Metal;
					material.ior = 1.0f;
					material.fuzz = randomFloat(0.0f, 0.5f);
					material.albedo = randomVec3(0.5f, 1.0f);
                } else {
					material.type = Material::Type::Dielectric;
					material.ior = 1.5f;
					material.fuzz = 0.0f;
					material.albedo = glm::vec3(1.0f, 1.0f, 1.0f);
                }

				_balls.push_back(Sphere(center, randomFloat(0.15f, 0.2f)));
				_ballMaterials.push_back(material);
            }
        }
    }
	
	// init three big sphere
	_balls.push_back(Sphere(glm::vec3(4.0f, 1.0f, 5.0f), 1.0f));
	_ballMaterials.push_back(Material(Material::Type::Dielectric, 1.5f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f)));
	
	_balls.push_back(Sphere(glm::vec3(-8.0f, 2.0f, 14.0f), 2.0f));
	_ballMaterials.push_back(Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.2f, 0.4f, 0.8f)));
	
	_balls.push_back(Sphere(glm::vec3(3.0f, 3.0f, -8.0f), 2.0f));
	_ballMaterials.push_back(Material(Material::Type::Metal, 1.0f, 0.0f, glm::vec3(0.7f, 0.6f, 0.5f)));
}

void RayTracing::createRenderScene(int index) {
	switch (index) {
		case 0: createScene1(); break;
		case 1: createScene2(); break;
		case 2: createScene3(); break;
		default: createScene3(); break;
	}
}

void RayTracing::createPrimitiveBuffer(const std::vector<Sphere>& spheres, 
                                       const std::vector<Model*> models, 
	                          		   const std::vector<glm::mat4>& transforms,
						               const std::vector<Material>& sphereMaterials, 
						               const std::vector<Material>& modelMaterials) {
	size_t totalPrimitives = 0;
	size_t totalTriangles = 0;
	size_t totalVertices = 0;
	totalPrimitives += spheres.size();
	for (const auto& model : models) {
		totalVertices += model->getVertices().size();
		totalTriangles += model->getIndices().size() / 3;
	}

	totalPrimitives += totalTriangles;
	size_t primitiveBufferSize = roundUp(totalPrimitives, BufferWidth);
	size_t materialBufferSize = roundUp(sphereMaterials.size() + modelMaterials.size(), BufferWidth);
	size_t vertexBufferSize = roundUp(totalVertices, BufferWidth);
	size_t triangleBufferSize = roundUp(totalTriangles, BufferWidth);
	std::vector<Vertex> vertices(vertexBufferSize);
	std::vector<Triangle> triangles(totalTriangles);
	std::vector<Primitive> primitives;
	std::vector<Material> materials(materialBufferSize);
	int primitiveCnt = 0;
	int materialCnt = 0;
	int vertexCnt = 0;
	int triangleCnt = 0;

	if (!spheres.empty()) {
		for (int i = 0; i < spheres.size(); ++i) {
			primitives.push_back(Primitive(Primitive::Type::Sphere, 
				primitiveCnt + i, materialCnt + i, const_cast<Sphere*>(&spheres[i])));
		}

		for (const auto& material : sphereMaterials) {
			materials[materialCnt++] = material;
		}

		std::vector<Sphere> sphereBuffer(roundUp(spheres.size(), BufferWidth));
		for (int i = 0; i < spheres.size(); ++i) {
			sphereBuffer[i] = spheres[i];
		}

		_sphereBuffer.reset(new Texture2D(GL_RGBA32F, BufferWidth, 
			getBufferHeight(sphereBuffer.size(), sizeof(Sphere), Sphere::getTexDataComponent()), 
			GL_RGBA, GL_FLOAT, sphereBuffer.data()));
		_sphereBuffer->bind();
		_sphereBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		_sphereBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		_sphereBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		_sphereBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		_sphereBuffer->unbind();
	} else {
		_sphereBuffer.reset(new Texture2D(GL_RGBA32F, BufferWidth, 
			getBufferHeight(1, sizeof(Sphere), Sphere::getTexDataComponent()), 
			GL_RGBA, GL_FLOAT, nullptr));
	}

	if (!models.empty()) {
		for (int i = 0; i < models.size(); ++i) {
			const auto& modelVertices = models[i]->getVertices();
			const auto& vertIndices = models[i]->getIndices();
			for (int j = 0, k = 0; j < vertIndices.size(); j += 3, ++k) {
				triangles[triangleCnt] = Triangle(vertIndices[j] + vertexCnt,
					vertIndices[j + 1] + vertexCnt,
					vertIndices[j + 2] + vertexCnt,
					vertices.data());
				primitives.push_back(Primitive(Primitive::Type::Triangle,
					triangleCnt,
					materialCnt + i,
					const_cast<Triangle*>(&triangles[triangleCnt])));
				triangleCnt++;
			}

			if (transforms[i] != glm::mat4(1.0f)) {
				const auto& transform = transforms[i];
				auto invTransposeTransform = glm::mat3(glm::transpose(glm::inverse(transform)));
				for (const auto& vertex : modelVertices) {
					vertices[vertexCnt++] = {transform * glm::vec4(vertex.position, 1.0f),
					                    invTransposeTransform * vertex.normal, 
										vertex.texCoord};
				}
			} else {
				for (const auto& vertex : modelVertices) {
					vertices[vertexCnt++] = vertex;
				}
			}
		}

		for (const auto& material : modelMaterials) {
			materials[materialCnt++] = material;
		}

		_vertexBuffer.reset(new Texture2D(
			GL_RGBA32F, BufferWidth, 
			getBufferHeight(vertexBufferSize, sizeof(Vertex), Sphere::getTexDataComponent()), 
			GL_RGBA, GL_FLOAT, vertices.data()));
		_vertexBuffer->bind();
		_vertexBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		_vertexBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		_vertexBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		_vertexBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		_vertexBuffer->unbind();

		std::vector<glm::ivec3> triangleIndex(triangleBufferSize);
		int triangleIndexCnt = 0;
		for (const auto& triangle : triangles) {
			triangleIndex[triangleIndexCnt++] = { triangle.v[0], triangle.v[1], triangle.v[2] };
		}

		_indexBuffer.reset(new Texture2D(
			GL_RGB32I, BufferWidth,
			getBufferHeight(triangleIndex.size(), sizeof(glm::ivec3), Triangle::getIndexTexDataComponent()),
			GL_RGB_INTEGER, GL_INT, triangleIndex.data()));
		_indexBuffer->bind();
		_indexBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		_indexBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		_indexBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		_indexBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		_indexBuffer->unbind();
	} else {
		_vertexBuffer.reset(new Texture2D(
			GL_RGBA32F, BufferWidth, 
			getBufferHeight(1, sizeof(Vertex), Sphere::getTexDataComponent()), 
			GL_RGBA, GL_FLOAT, nullptr));
		_indexBuffer.reset(new Texture2D(
			GL_RGB32I, BufferWidth,
			getBufferHeight(1, sizeof(glm::ivec3), Triangle::getIndexTexDataComponent()),
			GL_RGB_INTEGER, GL_INT, nullptr));
	}

	if (!materials.empty()) {
		for (auto& material : materials) {
			material.type = static_cast<Material::Type>(toFloatLayout(static_cast<int>(material.type)));
		}

		_materialBuffer.reset(new Texture2D(GL_RGB32F, BufferWidth, 
			getBufferHeight(materials.size(), sizeof(Material), Material::getTexDataComponent()),
			GL_RGB, GL_FLOAT, materials.data()));
		_materialBuffer->bind(0);
		_materialBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		_materialBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		_materialBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		_materialBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		_materialBuffer->unbind();
	} else {
		_materialBuffer.reset(new Texture2D(GL_RGB32F, BufferWidth, 
			getBufferHeight(1, sizeof(Material), Material::getTexDataComponent()),
			GL_RGB, GL_FLOAT, nullptr));
	}

	if (!primitives.empty()) {
		if (!_useBVH) {
			std::vector<ShaderPrimitive> orderedPrim(roundUp(primitives.size(), BufferWidth));
			int primCnt = 0;
			for (const auto& prim : primitives) {
				orderedPrim[primCnt++] = { static_cast<int>(prim.type), prim.shapeIdx, prim.materialIdx };
			}
			_bvhBuffer.reset(new Texture2D(GL_RGB32F, BufferWidth, 
				getBufferHeight(1, sizeof(BVHNode), BVHNode::getTexDataComponent()), 
				GL_RGB, GL_FLOAT, nullptr));

			_primitiveBuffer.reset(new Texture2D(GL_RGB32I, BufferWidth, 
				getBufferHeight(orderedPrim.size(), sizeof(ShaderPrimitive), Primitive::getTexDataComponent()), 
				GL_RGB_INTEGER, GL_INT, orderedPrim.data()));
			_primitiveBuffer->bind();
			_primitiveBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			_primitiveBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			_primitiveBuffer->unbind();
			_raytracingShader->use();
			_raytracingShader->setUniformInt("nPrimitives", static_cast<int>(primitives.size()));

		} else {
			// build BVH
			BVH bvh(primitives);
			auto& linearBVH = bvh.nodes;
			for (auto& node : linearBVH) {
				node.type = static_cast<BVHNode::Type>(toFloatLayout(static_cast<int>(node.type)));
				node.leftChild = toFloatLayout(node.leftChild);
				node.rightChild = toFloatLayout(node.rightChild);
			}

			std::vector<BVHNode> nodes(roundUp(linearBVH.size(), BufferWidth));
			int nodeCnt = 0;
			for (const auto& node : linearBVH) {
				nodes[nodeCnt++] = node;
			}

			_bvhBuffer.reset(new Texture2D(GL_RGB32F, BufferWidth, 
				getBufferHeight(nodes.size(), sizeof(BVHNode), BVHNode::getTexDataComponent()), 
				GL_RGB, GL_FLOAT, nodes.data()));
			_bvhBuffer->bind();
			_bvhBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			_bvhBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			_bvhBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			_bvhBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			_bvhBuffer->unbind();

			std::vector<ShaderPrimitive> orderedPrim(roundUp(bvh.orderedPrimitives.size(), BufferWidth));
			int primCnt = 0;
			for (const auto& prim : bvh.orderedPrimitives) {
				orderedPrim[primCnt++] = { static_cast<int>(prim.type), prim.shapeIdx, prim.materialIdx };
			}

			_primitiveBuffer.reset(new Texture2D(GL_RGB32I, BufferWidth, 
				getBufferHeight(orderedPrim.size(), sizeof(ShaderPrimitive), Primitive::getTexDataComponent()), 
				GL_RGB_INTEGER, GL_INT, orderedPrim.data()));
			_primitiveBuffer->bind();
			_primitiveBuffer->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			_primitiveBuffer->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			_primitiveBuffer->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			_primitiveBuffer->unbind();
		}
	}

	std::cout << "Scene Statistics" << std::endl;
	std::cout << "+ Spheres: " << spheres.size() << std::endl;
	std::cout << "+ Models:  "  << models.size() << std::endl;
	std::cout << "  + vertices:  " << totalVertices << std::endl;
	std::cout << "  + triangles: " << totalTriangles << std::endl;
}

void RayTracing::createScene1() {
	_camera->transform.position = glm::vec3(0.0f, 0.0f, 12.0f);
	_camera->transform.lookAt(glm::vec3(0.0f));

	_useBVH = false;

	createPrimitiveBuffer(
		{
			Sphere(glm::vec3(0.0f, 0.0f, 0.0f), 1.5f),
			Sphere(glm::vec3(4.0f, 0.0f, 0.0f), 1.5f),
			Sphere(glm::vec3(-4.0f, 0.0f, 0.0f), 1.5f)
		}, 
		{},
		{},
		{
			Material(Material::Type::Dielectric, 1.5f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f)),
			Material(Material::Type::Metal, 1.0f, 0.0f, glm::vec3(0.7f, 0.6f, 0.5f)),
			Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.8f, 0.4f, 0.2f))
		},
		{});
}

void RayTracing::createScene2() {
	_camera->transform.position = glm::vec3(15.0f, 3.0f, 4.0f);
	_camera->transform.lookAt(glm::vec3(0.0f));

	_useBVH = true;

	createPrimitiveBuffer(
		_balls, 
		{},
		{},
		_ballMaterials,
		{});
}

void RayTracing::createScene3() {
	_camera->transform.position = glm::vec3(15.0f, 3.0f, 4.0f);
	_camera->transform.lookAt(glm::vec3(0.0f));

	glm::mat4 scaleT = glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 0.6f, 0.6f));
	glm::mat4 rotateT = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	_useBVH = true;

	std::vector<glm::mat4> transformations = {
		rotateT * scaleT,
		glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.0f,  2.0f)) * rotateT * scaleT,
		glm::translate(glm::mat4(1.0f), glm::vec3( 4.0f, 0.0f, -2.0f)) * rotateT * scaleT
	};

	std::vector<Material> modelMaterials = {
		Material(Material::Type::Dielectric, 1.5f, 0.0f, glm::vec3(1.0f, 1.0f, 1.0f)),
		Material(Material::Type::Metal,      1.0f, 0.0f, glm::vec3(0.7f, 0.6f, 0.5f)),
		Material(Material::Type::Lambertian, 1.0f, 0.0f, glm::vec3(0.8f, 0.4f, 0.2f))
	};

	createPrimitiveBuffer(
		_balls, 
		{ _lucy.get(), _lucy.get(), _lucy.get() },
		transformations,
		_ballMaterials,
		modelMaterials);
}

int RayTracing::toFloatLayout(int v) {
	union {
		float f;
		int i;
	} tmp;
	tmp.f = static_cast<float>(v);
	return tmp.i;
}

size_t RayTracing::roundUp(size_t val, size_t number) {
	return ((val + number - 1) / number) * number;
}