#include <iostream>
#include <limits>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

#include <tiny_gltf.h>

#include "model.h"
#include "debug_print.h"

Model::Model(const std::string& filepath) {
	load(filepath);
}

Model::~Model() {
	cleanup();
}

void Model::reload(const std::string& filepath) {
	cleanup();
	load(filepath);
}

std::vector<Node*> Model::getRootNodes() {
	return _rootNodes;
}

void Model::load(const std::string& filepath) {
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;

	size_t index = filepath.find_last_of('.');
	if (index == std::string::npos) {
		throw std::runtime_error("find file extension name failure: " + filepath);
	}

	std::string ext = filepath.substr(index + 1);

	bool result;
	std::string err, warn;
	if (ext == "glb") {
		result = gltfContext.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath.c_str());
	} else {
		result = gltfContext.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath.c_str());
	}

	if (!warn.empty()) {
		std::cerr << "[Warn]" << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << "[Err]" << err << std::endl;
	}

	if (!result) {
		throw std::runtime_error("load " + filepath + " failure");
	} else {
		std::cout << "load " << filepath << " success" << std::endl;
	}

	/* load the default scene if it exists, or scene index 0 */
	const int sceneIndex = gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0;
	const tinygltf::Scene& scene = gltfModel.scenes[sceneIndex];
	std::cout << "+ sceneIndex: " << sceneIndex << std::endl;

	/* reserve memory for vertices and indices buffer */
	size_t vertexCount = 0;
	size_t indexCount = 0;
	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		auto result = getNodeProps(gltfModel.nodes[scene.nodes[i]], gltfModel);
		vertexCount += result.first;
		indexCount += result.second;
	}

	std::cout << "+ vertexCount: " << vertexCount << std::endl;
	std::cout << "+ indexCount: " << indexCount << std::endl;

	_vertices.reserve(vertexCount);
	_indices.reserve(indexCount);

	createGraphicResources(vertexCount, indexCount);

	loadSamplers(gltfModel);

	loadTextures(gltfModel);
	printTextures();

	loadMaterials(gltfModel);
	printMaterials();

	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		loadNode(nullptr, gltfModel.nodes[scene.nodes[i]], scene.nodes[i], gltfModel);
	}
	printNodeHierachy();

	if (!gltfModel.animations.empty()) {
		loadAnimations(gltfModel);
	}

	if (!gltfModel.skins.empty()) {
		loadSkins(gltfModel);
	}

	if (!gltfModel.extensionsUsed.empty()) {
		std::cout << "Extension is not currently supported" << std::endl;
	}

	updateGraphicResources();
}

void Model::loadSamplers(const tinygltf::Model& gltfModel) {
	// always set _samplers[0] as a default sampler
	std::unique_ptr<Sampler> defaultSampler { new Sampler };
	defaultSampler->setInt(GL_TEXTURE_MIN_FILTER, getFilterMode(-1));
	defaultSampler->setInt(GL_TEXTURE_MAG_FILTER, getFilterMode(-1));
	defaultSampler->setInt(GL_TEXTURE_WRAP_S, getWrapMode(-1));
	defaultSampler->setInt(GL_TEXTURE_WRAP_T, getWrapMode(-1));
	_samplers.push_back(std::move(defaultSampler));

	// get samplers from the gltfModel specification
	for (const auto& gltfSampler : gltfModel.samplers) {
		std::unique_ptr<Sampler> sampler { new Sampler };

		// filter mode
		sampler->setInt(GL_TEXTURE_MIN_FILTER, getFilterMode(gltfSampler.minFilter));
		sampler->setInt(GL_TEXTURE_MAG_FILTER, getFilterMode(gltfSampler.magFilter));

		// wrap mode
		sampler->setInt(GL_TEXTURE_WRAP_S, getWrapMode(gltfSampler.wrapS));
		sampler->setInt(GL_TEXTURE_WRAP_T, getWrapMode(gltfSampler.wrapT));

		_samplers.push_back(std::move(sampler));
	}
}

void Model::loadTextures(const tinygltf::Model& gltfModel) {
	// get textures from the gltfModel specification
	for (const tinygltf::Texture& gltfTexture : gltfModel.textures) {
		const tinygltf::Image& gltfImage = gltfModel.images[gltfTexture.source];

		GLint internalformat = GL_RGBA;

		GLint format = GL_RGBA;
		switch (gltfImage.component) {
			case 1: format = GL_RED; break;
			case 2: format = GL_RG; break;
			case 3: format = GL_RGB; break;
			case 4: format = GL_RGBA; break;
			default: throw std::runtime_error("unsupported image format");
		}

		GLenum type = GL_UNSIGNED_BYTE;
		switch (gltfImage.bits) {
			case 8: type = GL_UNSIGNED_BYTE; break;
			case 16: type = GL_UNSIGNED_SHORT; break;
			default: throw std::runtime_error("unsupported image data type");
		}

		std::unique_ptr<Texture2D> texture { new ImageTexture2D(
				gltfImage.image.data(),
				gltfImage.width,
				gltfImage.height,
				gltfImage.component,
				internalformat,
				format,
				type,
				gltfImage.uri) };

		if (gltfTexture.sampler != -1) {
			GLenum filterMode = getFilterMode(gltfModel.samplers[gltfTexture.sampler].minFilter);
			if (filterMode == GL_NEAREST_MIPMAP_NEAREST ||
				filterMode == GL_LINEAR_MIPMAP_NEAREST ||
				filterMode == GL_NEAREST_MIPMAP_LINEAR ||
				filterMode == GL_LINEAR_MIPMAP_LINEAR) {
				texture->bind();
				texture->generateMipmap();
				texture->unbind();
			}
		}

		_textures.emplace_back(std::move(texture));
	}
}

void Model::loadMaterials(const tinygltf::Model& gltfModel) {
	// always set the _materials[0] as a default material
	_materials.emplace_back(new PbrMaterial());
	_materials[0]->name = "defaultMaterial";

	for (const tinygltf::Material& gltfMaterial : gltfModel.materials) {
		std::unique_ptr<PbrMaterial> material { new PbrMaterial };
		material->name = gltfMaterial.name;

		// double sided
		material->doubleSided = gltfMaterial.doubleSided;

		// alpha cutoff
		material->alphaCutoff = static_cast<float>(gltfMaterial.alphaCutoff);

		// alpha mode
		if (gltfMaterial.alphaMode == "BLEND") {
			material->alphaMode = PbrMaterial::AlphaMode::Blend;
			// play a thick to discard pixels with very some alpha, even this is blend mode
			material->alphaCutoff = 0.05f;
		} else if (gltfMaterial.alphaMode == "MASK") {
			material->alphaMode = PbrMaterial::AlphaMode::Mask;
		} else if (gltfMaterial.alphaMode == "OPAQUE") {
			material->alphaMode = PbrMaterial::AlphaMode::Opaque;
		} else {
			throw std::runtime_error("unsupported material alpha mode: " + gltfMaterial.alphaMode);
		}

		int textureIndex = -1;
		int samplerIndex = -1;

		// albedo
		material->albedoFactor = glm::make_vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor.data());

		textureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
		if (textureIndex >= 0) {
			material->albedoMap = _textures[textureIndex].get();
			material->texCoordSets.albedo = gltfMaterial.pbrMetallicRoughness.baseColorTexture.texCoord;

			samplerIndex = gltfModel.textures[textureIndex].sampler;
			if (samplerIndex >= 0) {
				material->albeodoSampler = _samplers[samplerIndex + 1].get();
			} else {
				material->albeodoSampler = _samplers[0].get();
			}
		}

		// metallic
		material->metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
		
		textureIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
		if (textureIndex >= 0) {
			material->metallicMap = _textures[textureIndex].get();
			material->texCoordSets.metallic = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

			samplerIndex = gltfModel.textures[textureIndex].sampler;
			if (samplerIndex >= 0) {
				material->metallicSampler = _samplers[samplerIndex + 1].get();
			} else {
				material->metallicSampler = _samplers[0].get();
			}
		}

		// roughness
		material->roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);

		textureIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
		if (textureIndex >= 0) {
			material->roughnessMap = _textures[textureIndex].get();
			material->texCoordSets.roughness = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

			samplerIndex = gltfModel.textures[textureIndex].sampler;
			if (samplerIndex >= 0) {
				material->roughnessSampler = _samplers[samplerIndex + 1].get();
			} else {
				material->roughnessSampler = _samplers[0].get();
			}
		}
		
		// normal
		textureIndex = gltfMaterial.normalTexture.index;
		if (textureIndex >= 0) {
			material->normalMap = _textures[textureIndex].get();
			material->texCoordSets.normal = gltfMaterial.normalTexture.texCoord;

			samplerIndex = gltfModel.textures[textureIndex].sampler;
			if (samplerIndex >= 0) {
				material->normalSampler = _samplers[samplerIndex + 1].get();
			} else {
				material->normalSampler = _samplers[0].get();
			}
		}

		// occlusion
		material->occlusionStrength = 1.0f;

		textureIndex = gltfMaterial.occlusionTexture.index;
		if (textureIndex >= 0) {
			material->occlusionMap = _textures[textureIndex].get();
			material->texCoordSets.occlusion = gltfMaterial.occlusionTexture.texCoord;

			samplerIndex = gltfModel.textures[textureIndex].sampler;
			if (samplerIndex >= 0) {
				material->occlusionSampler = _samplers[samplerIndex + 1].get();
			} else {
				material->occlusionSampler = _samplers[0].get();
			}
		}

		// emissive
		material->emissiveFactor = glm::make_vec4(gltfMaterial.emissiveFactor.data());
		
		textureIndex = gltfMaterial.emissiveTexture.index;
		if (textureIndex >= 0) {
			material->emissiveMap = _textures[textureIndex].get();
			material->texCoordSets.emissive = gltfMaterial.emissiveTexture.texCoord;

			samplerIndex = gltfModel.textures[textureIndex].sampler;
			if (samplerIndex >= 0) {
				material->emissiveSampler = _samplers[samplerIndex + 1].get();
			} else {
				material->emissiveSampler = _samplers[0].get();
			}
		}

		_materials.push_back(std::move(material));
	}
}

void Model::loadAnimations(const tinygltf::Model& gltfModel) {
	std::cout << "Animation is not currently supported" << std::endl;
}

void Model::loadSkins(const tinygltf::Model& gltfModel) {
	std::cout << "Skin is not currently supported" << std::endl;
}

void Model::loadNode(
	Node* parent,
	const tinygltf::Node& gltfNode,
	uint32_t nodeIndex,
	const tinygltf::Model& gltfModel) {
	std::unique_ptr<Node> node{ new Node };

	// set meta info
	node->name = gltfNode.name;

	// set relation in scenegraph
	node->index = nodeIndex;
	node->parent = parent;

	// set transform component
	if (gltfNode.translation.size() == 3) {
		node->transform.position = glm::make_vec3(gltfNode.translation.data());
	}

	if (gltfNode.rotation.size() == 4) {
		node->transform.rotation = glm::make_quat(gltfNode.rotation.data());
	}

	if (gltfNode.scale.size() == 3) {
		node->transform.scale = glm::make_vec3(gltfNode.scale.data());
	}

	if (gltfNode.matrix.size() == 16) {
		node->transform.setFromTRS(glm::make_mat4x4(gltfNode.matrix.data()));
	}

	// process mesh
	if (gltfNode.mesh >= 0) {
		const tinygltf::Mesh& gltfMesh = gltfModel.meshes[gltfNode.mesh];
		for (const auto& gltfPrimitive : gltfMesh.primitives) {
			size_t count = 0;
			const uint32_t vertexStart = static_cast<uint32_t>(_vertices.size());
			const uint32_t indexStart = static_cast<uint32_t>(_indices.size());

			// parse vertices position 
			size_t vertexCount = 0;
			const float* positionBuffer = nullptr;
			int positionByteStride = 0;
			if (getAttributeBufferInfo<float>(
				gltfModel, gltfPrimitive, "POSITION", positionBuffer, positionByteStride, vertexCount)) {
				if (positionByteStride == -1) {
					throw std::runtime_error("illegal position byte stride");
				}
			} else {
				throw std::runtime_error("find position data failure");
			}

			// parse vertices normal
			const float* normalBuffer = nullptr;
			int normalByteStride = 0;
			if (getAttributeBufferInfo<float>(
				gltfModel, gltfPrimitive, "NORMAL", normalBuffer, normalByteStride, count)) {
				if (normalByteStride == -1) {
					throw std::runtime_error("illegal normal byte stride");
				}
			}

			// parse vertex texCoords0
			const float* texCoord0Buffer = nullptr;
			int texCoord0ByteStride = 0;
			if (getAttributeBufferInfo<float>(
				gltfModel, gltfPrimitive, "TEXCOORD_0", texCoord0Buffer, texCoord0ByteStride, count)) {
				if (texCoord0ByteStride == -1) {
					throw std::runtime_error("illegal texCoord0 byte stride");
				}
			}

			// parse vertex texCoords1
			const float* texCoord1Buffer = nullptr;
			int texCoord1ByteStride = 0;
			if (getAttributeBufferInfo<float>(
				gltfModel, gltfPrimitive, "TEXCOORD_1", texCoord1Buffer, texCoord1ByteStride, count)) {
				if (texCoord1ByteStride == -1) {
					throw std::runtime_error("illegal texCoord1 byte stride");
				} else if (texCoord1ByteStride == 0) {
					texCoord1ByteStride = tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2) * sizeof(float);
				}
			}

			// assemble vertices data
			for (size_t i = 0; i < vertexCount; ++i) {
				Vertex v;
				v.position = glm::make_vec3(positionBuffer + i * positionByteStride / sizeof(float));

				v.normal = glm::normalize(normalBuffer ?
					glm::make_vec3(normalBuffer + i * normalByteStride / sizeof(float)) :
					glm::vec3(1.0f, 1.0f, 1.0f));

				v.texCoord0 = texCoord0Buffer ?
					glm::make_vec2(texCoord0Buffer + i * texCoord0ByteStride / sizeof(float)) : 
					glm::vec2(0.0f, 0.0f);

				v.texCoord1 = texCoord1Buffer ?
					glm::make_vec2(texCoord1Buffer + i * texCoord1ByteStride / sizeof(float)) :
					glm::vec2(0.0f, 0.0f);

				_vertices.emplace_back(v);
			}

			// indices
			size_t indexCount = 0;
			if (gltfPrimitive.indices >= 0) {
				const tinygltf::Accessor& accessor = gltfModel.accessors[gltfPrimitive.indices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				indexCount = accessor.count;
				const void* data = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
						const uint32_t* buf = static_cast<const uint32_t*>(data);
						for (size_t i = 0; i < indexCount; ++i) {
							_indices.push_back(buf[i] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
						const uint16_t* buf = static_cast<const uint16_t*>(data);
						for (size_t i = 0; i < indexCount; ++i) {
							_indices.push_back(buf[i] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
						const uint8_t* buf = static_cast<const uint8_t*>(data);
						for (size_t i = 0; i < indexCount; ++i) {
							_indices.push_back(buf[i] + vertexStart);
						}
						break;
					}
					default:
						throw std::runtime_error("unsupported index data type");
				}
			}

			Primitive primitive = {
				_vao,
				vertexStart,
				static_cast<uint32_t>(vertexCount),
				indexStart, 
				static_cast<uint32_t>(indexCount), 
				gltfPrimitive.material >= 0 ? _materials[gltfPrimitive.material + 1].get() : _materials[0].get()
			};
			node->primitives.push_back(primitive);
		}
	}

	if (parent) {
		parent->children.push_back(node.get());
	} else {
		_rootNodes.push_back(node.get());
	}

	// process child nodes
	for (const auto childIndex : gltfNode.children) {
		loadNode(node.get(), gltfModel.nodes[childIndex], childIndex, gltfModel);
	}

	_nodes.push_back(std::move(node));
}

void Model::createGraphicResources(size_t vertexCount, size_t indexCount) {
	// create a vertex array object
	glGenVertexArrays(1, &_vao);
	// create a vertex buffer object
	glGenBuffers(1, &_vbo);
	// create a element array buffer
	glGenBuffers(1, &_ibo);

	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, NULL, GL_STATIC_DRAW);

	if (indexCount > 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), NULL, GL_STATIC_DRAW);
	}

	// specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord0));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord1));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);
}

void Model::updateGraphicResources() {
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * _vertices.size(), _vertices.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(uint32_t) * _indices.size(), _indices.data());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Model::cleanup() {
	_nodes.clear();
	_rootNodes.clear();

	_materials.clear();
	_samplers.clear();
	_textures.clear();

	_vertices.clear();
	_indices.clear();

	if (_vao != 0) {
		glDeleteVertexArrays(1, &_vao);
		_vao = 0;
	}

	if (_vbo != 0) {
		glDeleteBuffers(1, &_vbo);
		_vbo = 0;
	}

	if (_ibo != 0) {
		glDeleteBuffers(1, &_ibo);
		_ibo = 0;
	}
}

std::pair<size_t, size_t> Model::getNodeProps(
	const tinygltf::Node& node,
	const tinygltf::Model& model) {

	size_t vertexCount = 0;
	size_t indexCount = 0;

	if (node.mesh >= 0) {
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {
			const auto& primitive = mesh.primitives[i];
			vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
			if (primitive.indices >= 0) {
				indexCount += model.accessors[primitive.indices].count;
			}
		}
	}

	for (size_t i = 0; i < node.children.size(); ++i) {
		auto result = getNodeProps(model.nodes[node.children[i]], model);
		vertexCount += result.first;
		indexCount += result.second;
	}

	return { vertexCount, indexCount };
}

template <typename T>
bool Model::getAttributeBufferInfo(
	const tinygltf::Model& gltfModel,
	const tinygltf::Primitive& gltfPrimitive,
	const std::string& name,
	const T*& data,
	int& byteStride,
	size_t& count) {
	const auto iter = gltfPrimitive.attributes.find(name);
	if (iter == gltfPrimitive.attributes.end()) {
		return false;
	}

	const tinygltf::Accessor& accessor = gltfModel.accessors[iter->second];
	const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];

	data = reinterpret_cast<const T*>(&(
		gltfModel.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));

	count = accessor.count;

	byteStride = accessor.ByteStride(bufferView);

	return true;
}

Node* Model::getNodeByIndex(int index) const {
	for (const auto& node : _nodes) {
		if (node->index == index) {
			return node.get();
		}
	}

	return nullptr;
}

int Model::getSamplerIndex(const Sampler* sampler) const {
	for (size_t i = 0; i < _samplers.size(); ++i) {
		if (sampler == _samplers[i].get()) {
			return static_cast<int>(i);
		}
	}

	return -1;
}

int Model::getTextureIndex(const Texture* texture) const {
	for (size_t i = 0; i < _textures.size(); ++i) {
		if (texture == _textures[i].get()) {
			return static_cast<int>(i);
		}
	}

	return -1;
}

GLenum Model::getFilterMode(int gltfFilterMode) {
	switch (gltfFilterMode) {
		case 9728: return GL_NEAREST;
		case 9729: return GL_LINEAR;
		case 9984: return GL_NEAREST_MIPMAP_NEAREST;
		case 9985: return GL_LINEAR_MIPMAP_NEAREST;
		case 9986: return GL_NEAREST_MIPMAP_LINEAR;
		case 9987: return GL_LINEAR_MIPMAP_LINEAR;
		default: return GL_LINEAR;
	}
}

GLenum Model::getWrapMode(int gltfWrapMode) {
	switch (gltfWrapMode) {
		case 10497: return GL_REPEAT;
		case 33071: return GL_CLAMP_TO_EDGE;
		case 33648: return GL_MIRRORED_REPEAT;
		default: return GL_REPEAT;
	}
}

void Model::printTextures() const {
	std::cout << "+ Textures:" << std::endl;
	for (size_t i = 0; i < _textures.size(); ++i) {
		std::cout << "  + texture[" << i << "]: ";
		ImageTexture2D* tex2D = dynamic_cast<ImageTexture2D*>(_textures[i].get());
		if (tex2D) {
			std::cout << tex2D->getUri() << std::endl;
		} else {
			std::cout << std::endl;
		}
	}
}

void Model::printMaterials() const {
	std::cout << "+ Materials" << std::endl;
	for (size_t i = 0; i < _materials.size(); ++i) {
		std::string alphaMode;
		switch (_materials[i]->alphaMode) {
			case PbrMaterial::AlphaMode::Mask: alphaMode = "Mask"; break;
			case PbrMaterial::AlphaMode::Blend: alphaMode = "Blend"; break;
			case PbrMaterial::AlphaMode::Opaque: alphaMode = "Opaque"; break;
		}

		std::cout << "  + material[" << i << "]" << "\n";
		std::cout << "    + name:        "   << _materials[i]->name        << "\n";
		std::cout << "    + doubleSided: "   << _materials[i]->doubleSided << "\n";
		std::cout << "    + alphaMode:   "   << alphaMode                  << "\n";
		std::cout << "    + alphaCutoff: "   << _materials[i]->alphaCutoff << "\n";
		std::cout << "    + albedo: "                                      << "\n";
		std::cout << "      + factor: "      << _materials[i]->albedoFactor                      << "\n";
		std::cout << "      + texture: "     << getTextureIndex(_materials[i]->albedoMap)        << "\n";
		std::cout << "      + sampler: "     << getSamplerIndex(_materials[i]->albeodoSampler)   << "\n";
		std::cout << "      + texCoordSet: " << _materials[i]->texCoordSets.albedo               << "\n";
		std::cout << "    + metallic: "                                    << "\n";
		std::cout << "      + factor: "      << _materials[i]->metallicFactor                    << "\n";
		std::cout << "      + texture: "     << getTextureIndex(_materials[i]->metallicMap)      << "\n";
		std::cout << "      + sampler: "     << getSamplerIndex(_materials[i]->metallicSampler)  << "\n";
		std::cout << "      + texCoordSet: " << _materials[i]->texCoordSets.metallic             << "\n";
		std::cout << "    + roughness: "                                   << "\n";
		std::cout << "      + factor: "      << _materials[i]->metallicFactor                    << "\n";
		std::cout << "      + texture: "     << getTextureIndex(_materials[i]->roughnessMap)     << "\n";
		std::cout << "      + sampler: "     << getSamplerIndex(_materials[i]->roughnessSampler) << "\n";
		std::cout << "      + texCoordSet: " << _materials[i]->texCoordSets.roughness            << "\n";
		std::cout << "    + normal: "                                      << "\n";
		std::cout << "      + texture: "     << getTextureIndex(_materials[i]->normalMap)        << "\n";
		std::cout << "      + sampler: "     << getSamplerIndex(_materials[i]->normalSampler)    << "\n";
		std::cout << "      + texCoordSet: " << _materials[i]->texCoordSets.normal               << "\n";
		std::cout << "    + occlusion: "                                   << "\n";
		std::cout << "      + strength:"     << _materials[i]->occlusionStrength                 << "\n";
		std::cout << "      + texture: "     << getTextureIndex(_materials[i]->occlusionMap)     << "\n";
		std::cout << "      + sampler: "     << getSamplerIndex(_materials[i]->occlusionSampler) << "\n";
		std::cout << "      + texCoordSet: " << _materials[i]->texCoordSets.metallic             << "\n";
		std::cout << "    + emissive: "                                    << "\n";
		std::cout << "      + factor: "      << _materials[i]->emissiveFactor                    << "\n";
		std::cout << "      + texture: "     << getTextureIndex(_materials[i]->emissiveMap)      << "\n";
		std::cout << "      + sampler: "     << getSamplerIndex(_materials[i]->emissiveSampler)  << "\n";
		std::cout << "      + texCoordSet: " << _materials[i]->texCoordSets.emissive             << "\n";
	}
}

void Model::printNodeHierachy() const {
	std::cout << "+ Node Hierachy" << std::endl;
	for (const Node* node : _rootNodes) {
		printNode(node, 1);
	}
}

void Model::printNode(const Node* node, int depth) const {
	std::string indentation = indent(depth * 2);
	std::cout << indentation << "+ name:      " << node->name << std::endl;
	std::cout << indentation << "+ parent:    " << node->parent << std::endl;
	std::cout << indentation << "+ position:  " << node->transform.position << std::endl;
	std::cout << indentation << "+ rotation:  " << node->transform.rotation << std::endl;
	std::cout << indentation << "+ scale:     " << node->transform.scale << std::endl;
	std::cout << indentation << "+ nChildren: " << node->children.size() << std::endl;

	for (size_t i = 0; i < node->children.size(); ++i) {
		printNode(node->children[i], depth + 1);
	}
}