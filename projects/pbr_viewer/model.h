#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glad/glad.h>

#include <tiny_gltf.h>

#include "../base/glsl_program.h"
#include "../base/sampler.h"
#include "../base/texture2d.h"

#include "vertex.h"
#include "node.h"
#include "animation.h"
#include "material.h"

struct Model {
public:
	Model(const std::string& filepath);

	~Model();

	std::vector<Node*> getRootNodes();

	void reload(const std::string& filepath);
	
private:
	std::vector<std::unique_ptr<Node>> _nodes;
	std::vector<Node*> _rootNodes;

	std::vector<std::unique_ptr<PbrMaterial>> _materials;
	std::vector<std::unique_ptr<Sampler>> _samplers;
	std::vector<std::unique_ptr<Texture>> _textures;

	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	
	GLuint _vao = 0;
	GLuint _vbo = 0;
	GLuint _ibo = 0;

	void load(const std::string& filepath);

	void loadSamplers(const tinygltf::Model& gltfModel);

	void loadTextures(const tinygltf::Model& gltfModel);

	void loadMaterials(const tinygltf::Model& gltfModel);

	void loadAnimations(const tinygltf::Model& gltfModel);

	void loadSkins(const tinygltf::Model& gltfModel);

	void loadNode(
		Node* parent, 
		const tinygltf::Node& node, 
		uint32_t nodeIndex, 
		const tinygltf::Model& model);

	void cleanup();

	std::pair<size_t, size_t> getNodeProps(
		const tinygltf::Node& node,
		const tinygltf::Model& model);

	template <typename T>
	static bool getAttributeBufferInfo(
		const tinygltf::Model& gltfModel,
		const tinygltf::Primitive& gltfPrimitive,
		const std::string& name,
		const T*& data,
		int& byteStride,
		size_t& count);

	Node* getNodeByIndex(int index) const;

	int getSamplerIndex(const Sampler* sampler) const;

	int getTextureIndex(const Texture* texture) const;

	void createGraphicResources(size_t vertexCount, size_t indexCount);

	void updateGraphicResources();

	static GLenum getFilterMode(int gltfFilterMode);

	static GLenum getWrapMode(int gltfWrapMode);

	void printTextures() const;

	void printMaterials() const;

	void printNodeHierachy() const;

	void printNode(const Node* node, int depth) const;
};