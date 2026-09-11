#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <stb_image.h>
#include <tiny_gltf_v3.h>

#include "debug_print.h"
#include "model.h"

namespace {

std::string toString(tg3_str value) {
    return value.data ? std::string(value.data, value.len) : std::string{};
}

const tg3_str_int_pair* findAttribute(const tg3_primitive& primitive, const std::string& name) {
    for (uint32_t i = 0; i < primitive.attributes_count; ++i) {
        if (tg3_str_equals_cstr(primitive.attributes[i].key, name.c_str())) {
            return &primitive.attributes[i];
        }
    }
    return nullptr;
}

std::vector<uint8_t> decodeBase64(const std::string& uri) {
    const size_t comma = uri.find(',');
    if (comma == std::string::npos || uri.substr(0, comma).find(";base64") == std::string::npos) {
        throw std::runtime_error("unsupported image data URI");
    }

    static const std::string alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> bytes;
    int value = 0;
    int bits = -8;
    for (size_t i = comma + 1; i < uri.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(uri[i]);
        if (std::isspace(c)) continue;
        if (c == '=') break;
        const size_t position = alphabet.find(c);
        if (position == std::string::npos) throw std::runtime_error("invalid base64 image data URI");
        value = (value << 6) + static_cast<int>(position);
        bits += 6;
        if (bits >= 0) {
            bytes.push_back(static_cast<uint8_t>((value >> bits) & 0xff));
            bits -= 8;
        }
    }
    return bytes;
}

std::vector<uint8_t> readFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("read image file failure: " + path.string());
    const std::streamsize size = input.tellg();
    if (size < 0) throw std::runtime_error("read image file size failure: " + path.string());
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), size)) {
        throw std::runtime_error("read image file failure: " + path.string());
    }
    return bytes;
}

std::vector<uint8_t> imageBytes(
    const tg3_model& model, const tg3_image& image, const std::filesystem::path& basePath) {
    if (image.buffer_view >= 0) {
        const tg3_buffer_view& view = model.buffer_views[image.buffer_view];
        const tg3_buffer& buffer = model.buffers[view.buffer];
        return std::vector<uint8_t>(
            buffer.data.data + view.byte_offset, buffer.data.data + view.byte_offset + view.byte_length);
    }

    const std::string uri = toString(image.uri);
    if (uri.rfind("data:", 0) == 0) return decodeBase64(uri);
    if (uri.empty()) throw std::runtime_error("image has neither URI nor buffer view");
    return readFile(basePath / std::filesystem::path(uri));
}

std::string imageDescription(const tg3_image& image) {
    if (image.buffer_view >= 0) {
        return "<embedded bufferView " + std::to_string(image.buffer_view) + ">";
    }

    const std::string uri = toString(image.uri);
    if (uri.rfind("data:", 0) == 0) return "<embedded data URI>";
    return uri;
}

}  // namespace

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
    return m_rootNodes;
}

void Model::load(const std::string& filepath) {
    tinygltf3::Model model;
    tinygltf3::ErrorStack errors;
    tg3_parse_options options;
    tg3_parse_options_init(&options);
    const tg3_error_code result = tinygltf3::parse_file(model, errors, filepath.c_str(), &options);
    for (uint32_t i = 0; i < errors.count(); ++i) {
        const tg3_error_entry* error = errors.entry(i);
        std::cerr << "[TinyGLTF v3] " << (error->message ? error->message : "unknown error")
                  << std::endl;
    }
    if (result != TG3_OK) throw std::runtime_error("load " + filepath + " failure");
    const tg3_model& gltfModel = *model.get();
    std::cout << "load " << filepath << " success" << std::endl;

    /* load the default scene if it exists, or scene index 0 */
    if (gltfModel.scenes_count == 0) throw std::runtime_error("model contains no scenes");
    const int sceneIndex = gltfModel.default_scene >= 0 ? gltfModel.default_scene : 0;
    const tg3_scene& scene = gltfModel.scenes[sceneIndex];
    std::cout << "+ sceneIndex: " << sceneIndex << std::endl;

    /* reserve memory for vertices and indices buffer */
    size_t vertexCount = 0;
    size_t indexCount = 0;
    for (uint32_t i = 0; i < scene.nodes_count; ++i) {
        auto result = getNodeProps(gltfModel.nodes[scene.nodes[i]], gltfModel);
        vertexCount += result.first;
        indexCount += result.second;
    }

    std::cout << "+ vertexCount: " << vertexCount << std::endl;
    std::cout << "+ indexCount: " << indexCount << std::endl;

    m_vertices.reserve(vertexCount);
    m_indices.reserve(indexCount);

    createGraphicResources(vertexCount, indexCount);

    loadSamplers(gltfModel);

    loadTextures(gltfModel, filepath);
    printTextures();

    loadMaterials(gltfModel);
    printMaterials();

    for (uint32_t i = 0; i < scene.nodes_count; ++i) {
        loadNode(nullptr, gltfModel.nodes[scene.nodes[i]], scene.nodes[i], gltfModel);
    }
    printNodeHierachy();

    if (gltfModel.animations_count != 0) {
        loadAnimations(gltfModel);
    }

    if (gltfModel.skins_count != 0) {
        loadSkins(gltfModel);
    }

    if (gltfModel.extensions_used_count != 0) {
        std::cout << "Extension is not currently supported" << std::endl;
    }

    updateGraphicResources();
}

void Model::loadSamplers(const tg3_model& gltfModel) {
    // always set m_samplers[0] as a default sampler
    std::unique_ptr<Sampler> defaultSampler{new Sampler};
    defaultSampler->setInt(GL_TEXTURE_MIN_FILTER, getFilterMode(-1));
    defaultSampler->setInt(GL_TEXTURE_MAG_FILTER, getFilterMode(-1));
    defaultSampler->setInt(GL_TEXTURE_WRAP_S, getWrapMode(-1));
    defaultSampler->setInt(GL_TEXTURE_WRAP_T, getWrapMode(-1));
    m_samplers.push_back(std::move(defaultSampler));

    // get samplers from the gltfModel specification
    for (uint32_t i = 0; i < gltfModel.samplers_count; ++i) {
        const tg3_sampler& gltfSampler = gltfModel.samplers[i];
        std::unique_ptr<Sampler> sampler{new Sampler};

        // filter mode
        sampler->setInt(GL_TEXTURE_MIN_FILTER, getFilterMode(gltfSampler.min_filter));
        sampler->setInt(GL_TEXTURE_MAG_FILTER, getFilterMode(gltfSampler.mag_filter));

        // wrap mode
        sampler->setInt(GL_TEXTURE_WRAP_S, getWrapMode(gltfSampler.wrap_s));
        sampler->setInt(GL_TEXTURE_WRAP_T, getWrapMode(gltfSampler.wrap_t));

        m_samplers.push_back(std::move(sampler));
    }
}

void Model::loadTextures(const tg3_model& gltfModel, const std::string& filepath) {
    // get textures from the gltfModel specification
    const std::filesystem::path basePath = std::filesystem::path(filepath).parent_path();
    for (uint32_t i = 0; i < gltfModel.textures_count; ++i) {
        const tg3_texture& gltfTexture = gltfModel.textures[i];
        const tg3_image& gltfImage = gltfModel.images[gltfTexture.source];
        std::vector<uint8_t> bytes = imageBytes(gltfModel, gltfImage, basePath);
        if (bytes.empty() || bytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("invalid image data");
        }
        int width = 0, height = 0, channels = 0;
        stbi_uc* pixels = stbi_load_from_memory(
            bytes.data(), static_cast<int>(bytes.size()), &width, &height, &channels, 0);
        if (!pixels) throw std::runtime_error("decode image failure: " + toString(gltfImage.uri));
        GLenum format = GL_RGBA;
        switch (channels) {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            stbi_image_free(pixels);
            throw std::runtime_error("unsupported image format");
        }
        std::unique_ptr<Texture2D> texture{new ImageTexture2D(
            pixels, width, height, channels, static_cast<GLint>(format), format, GL_UNSIGNED_BYTE,
            imageDescription(gltfImage))};
        stbi_image_free(pixels);

        if (gltfTexture.sampler != -1) {
            GLenum filterMode = getFilterMode(gltfModel.samplers[gltfTexture.sampler].min_filter);
            if (filterMode == GL_NEAREST_MIPMAP_NEAREST || filterMode == GL_LINEAR_MIPMAP_NEAREST
                || filterMode == GL_NEAREST_MIPMAP_LINEAR
                || filterMode == GL_LINEAR_MIPMAP_LINEAR) {
                texture->bind();
                texture->generateMipmap();
                texture->unbind();
            }
        }

        m_textures.emplace_back(std::move(texture));
    }
}

void Model::loadMaterials(const tg3_model& gltfModel) {
    // always set the m_materials[0] as a default material
    m_materials.emplace_back(new PbrMaterial());
    m_materials[0]->name = "defaultMaterial";

    for (uint32_t i = 0; i < gltfModel.materials_count; ++i) {
        const tg3_material& gltfMaterial = gltfModel.materials[i];
        std::unique_ptr<PbrMaterial> material{new PbrMaterial};
        material->name = toString(gltfMaterial.name);

        // double sided
        material->doubleSided = gltfMaterial.double_sided != 0;

        // alpha cutoff
        material->alphaCutoff = static_cast<float>(gltfMaterial.alpha_cutoff);

        // alpha mode
        if (tg3_str_equals_cstr(gltfMaterial.alpha_mode, "BLEND")) {
            material->alphaMode = PbrMaterial::AlphaMode::Blend;
            // play a thick to discard pixels with very some alpha, even this is blend mode
            material->alphaCutoff = 0.05f;
        } else if (tg3_str_equals_cstr(gltfMaterial.alpha_mode, "MASK")) {
            material->alphaMode = PbrMaterial::AlphaMode::Mask;
        } else if (tg3_str_equals_cstr(gltfMaterial.alpha_mode, "OPAQUE")) {
            material->alphaMode = PbrMaterial::AlphaMode::Opaque;
        } else {
            throw std::runtime_error("unsupported material alpha mode: " + toString(gltfMaterial.alpha_mode));
        }

        int textureIndex = -1;
        int samplerIndex = -1;

        // albedo
        const tg3_pbr_metallic_roughness& pbr = gltfMaterial.pbr_metallic_roughness;
        material->albedoFactor = glm::vec4(
            static_cast<float>(pbr.base_color_factor[0]), static_cast<float>(pbr.base_color_factor[1]),
            static_cast<float>(pbr.base_color_factor[2]), static_cast<float>(pbr.base_color_factor[3]));

        textureIndex = pbr.base_color_texture.index;
        if (textureIndex >= 0) {
            material->albedoMap = m_textures[textureIndex].get();
            material->texCoordSets.albedo =
                pbr.base_color_texture.tex_coord;

            samplerIndex = gltfModel.textures[textureIndex].sampler;
            if (samplerIndex >= 0) {
                material->albeodoSampler = m_samplers[samplerIndex + 1].get();
            } else {
                material->albeodoSampler = m_samplers[0].get();
            }
        }

        // metallic
        material->metallicFactor =
            static_cast<float>(pbr.metallic_factor);

        textureIndex = pbr.metallic_roughness_texture.index;
        if (textureIndex >= 0) {
            material->metallicMap = m_textures[textureIndex].get();
            material->texCoordSets.metallic =
                pbr.metallic_roughness_texture.tex_coord;

            samplerIndex = gltfModel.textures[textureIndex].sampler;
            if (samplerIndex >= 0) {
                material->metallicSampler = m_samplers[samplerIndex + 1].get();
            } else {
                material->metallicSampler = m_samplers[0].get();
            }
        }

        // roughness
        material->roughnessFactor =
            static_cast<float>(pbr.roughness_factor);

        textureIndex = pbr.metallic_roughness_texture.index;
        if (textureIndex >= 0) {
            material->roughnessMap = m_textures[textureIndex].get();
            material->texCoordSets.roughness =
                pbr.metallic_roughness_texture.tex_coord;

            samplerIndex = gltfModel.textures[textureIndex].sampler;
            if (samplerIndex >= 0) {
                material->roughnessSampler = m_samplers[samplerIndex + 1].get();
            } else {
                material->roughnessSampler = m_samplers[0].get();
            }
        }

        // normal
        textureIndex = gltfMaterial.normal_texture.index;
        if (textureIndex >= 0) {
            material->normalMap = m_textures[textureIndex].get();
            material->texCoordSets.normal = gltfMaterial.normal_texture.tex_coord;

            samplerIndex = gltfModel.textures[textureIndex].sampler;
            if (samplerIndex >= 0) {
                material->normalSampler = m_samplers[samplerIndex + 1].get();
            } else {
                material->normalSampler = m_samplers[0].get();
            }
        }

        // occlusion
        material->occlusionStrength = static_cast<float>(gltfMaterial.occlusion_texture.strength);

        textureIndex = gltfMaterial.occlusion_texture.index;
        if (textureIndex >= 0) {
            material->occlusionMap = m_textures[textureIndex].get();
            material->texCoordSets.occlusion = gltfMaterial.occlusion_texture.tex_coord;

            samplerIndex = gltfModel.textures[textureIndex].sampler;
            if (samplerIndex >= 0) {
                material->occlusionSampler = m_samplers[samplerIndex + 1].get();
            } else {
                material->occlusionSampler = m_samplers[0].get();
            }
        }

        // emissive
        material->emissiveFactor = glm::vec4(
            static_cast<float>(gltfMaterial.emissive_factor[0]),
            static_cast<float>(gltfMaterial.emissive_factor[1]),
            static_cast<float>(gltfMaterial.emissive_factor[2]), 1.0f);

        textureIndex = gltfMaterial.emissive_texture.index;
        if (textureIndex >= 0) {
            material->emissiveMap = m_textures[textureIndex].get();
            material->texCoordSets.emissive = gltfMaterial.emissive_texture.tex_coord;

            samplerIndex = gltfModel.textures[textureIndex].sampler;
            if (samplerIndex >= 0) {
                material->emissiveSampler = m_samplers[samplerIndex + 1].get();
            } else {
                material->emissiveSampler = m_samplers[0].get();
            }
        }

        m_materials.push_back(std::move(material));
    }
}

void Model::loadAnimations(const tg3_model& gltfModel) {
    std::cout << "Animation is not currently supported" << std::endl;
}

void Model::loadSkins(const tg3_model& gltfModel) {
    std::cout << "Skin is not currently supported" << std::endl;
}

void Model::loadNode(
    Node* parent, const tg3_node& gltfNode, uint32_t nodeIndex, const tg3_model& gltfModel) {
    std::unique_ptr<Node> node{new Node};

    // set meta info
    node->name = toString(gltfNode.name);

    // set relation in scenegraph
    node->index = nodeIndex;
    node->parent = parent;

    // set transform component
    node->transform.position = glm::vec3(
        static_cast<float>(gltfNode.translation[0]), static_cast<float>(gltfNode.translation[1]),
        static_cast<float>(gltfNode.translation[2]));
    node->transform.rotation = glm::quat(
        static_cast<float>(gltfNode.rotation[3]), static_cast<float>(gltfNode.rotation[0]),
        static_cast<float>(gltfNode.rotation[1]), static_cast<float>(gltfNode.rotation[2]));
    node->transform.scale = glm::vec3(
        static_cast<float>(gltfNode.scale[0]), static_cast<float>(gltfNode.scale[1]),
        static_cast<float>(gltfNode.scale[2]));
    if (gltfNode.has_matrix) {
        glm::mat4 matrix(1.0f);
        for (int i = 0; i < 16; ++i) matrix[i / 4][i % 4] = static_cast<float>(gltfNode.matrix[i]);
        node->transform.setFromTRS(matrix);
    }

    // process mesh
    if (gltfNode.mesh >= 0) {
        const tg3_mesh& gltfMesh = gltfModel.meshes[gltfNode.mesh];
        for (uint32_t primitiveIndex = 0; primitiveIndex < gltfMesh.primitives_count; ++primitiveIndex) {
            const tg3_primitive& gltfPrimitive = gltfMesh.primitives[primitiveIndex];
            size_t count = 0;
            const uint32_t vertexStart = static_cast<uint32_t>(m_vertices.size());
            const uint32_t indexStart = static_cast<uint32_t>(m_indices.size());

            // parse vertices position
            size_t vertexCount = 0;
            const float* positionBuffer = nullptr;
            int positionByteStride = 0;
            if (getAttributeBufferInfo<float>(
                    gltfModel, gltfPrimitive, "POSITION", positionBuffer, positionByteStride,
                    vertexCount)) {
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
                    gltfModel, gltfPrimitive, "TEXCOORD_0", texCoord0Buffer, texCoord0ByteStride,
                    count)) {
                if (texCoord0ByteStride == -1) {
                    throw std::runtime_error("illegal texCoord0 byte stride");
                }
            }

            // parse vertex texCoords1
            const float* texCoord1Buffer = nullptr;
            int texCoord1ByteStride = 0;
            if (getAttributeBufferInfo<float>(
                    gltfModel, gltfPrimitive, "TEXCOORD_1", texCoord1Buffer, texCoord1ByteStride,
                    count)) {
                if (texCoord1ByteStride == -1) {
                    throw std::runtime_error("illegal texCoord1 byte stride");
                } else if (texCoord1ByteStride == 0) {
                    texCoord1ByteStride =
                        tg3_num_components(TG3_TYPE_VEC2) * sizeof(float);
                }
            }

            // assemble vertices data
            for (size_t i = 0; i < vertexCount; ++i) {
                Vertex v;
                v.position =
                    glm::make_vec3(positionBuffer + i * positionByteStride / sizeof(float));

                v.normal = glm::normalize(
                    normalBuffer
                        ? glm::make_vec3(normalBuffer + i * normalByteStride / sizeof(float))
                        : glm::vec3(1.0f, 1.0f, 1.0f));

                v.texCoord0 =
                    texCoord0Buffer
                        ? glm::make_vec2(texCoord0Buffer + i * texCoord0ByteStride / sizeof(float))
                        : glm::vec2(0.0f, 0.0f);

                v.texCoord1 =
                    texCoord1Buffer
                        ? glm::make_vec2(texCoord1Buffer + i * texCoord1ByteStride / sizeof(float))
                        : glm::vec2(0.0f, 0.0f);

                m_vertices.emplace_back(v);
            }

            // indices
            size_t indexCount = 0;
            if (gltfPrimitive.indices >= 0) {
                const tg3_accessor& accessor = gltfModel.accessors[gltfPrimitive.indices];
                const tg3_buffer_view& bufferView = gltfModel.buffer_views[accessor.buffer_view];
                const tg3_buffer& buffer = gltfModel.buffers[bufferView.buffer];

                indexCount = accessor.count;
                const void* data = buffer.data.data + accessor.byte_offset + bufferView.byte_offset;

                switch (accessor.component_type) {
                case TG3_COMPONENT_TYPE_UNSIGNED_INT: {
                    const uint32_t* buf = static_cast<const uint32_t*>(data);
                    for (size_t i = 0; i < indexCount; ++i) {
                        m_indices.push_back(buf[i] + vertexStart);
                    }
                    break;
                }
                case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    const uint16_t* buf = static_cast<const uint16_t*>(data);
                    for (size_t i = 0; i < indexCount; ++i) {
                        m_indices.push_back(buf[i] + vertexStart);
                    }
                    break;
                }
                case TG3_COMPONENT_TYPE_UNSIGNED_BYTE: {
                    const uint8_t* buf = static_cast<const uint8_t*>(data);
                    for (size_t i = 0; i < indexCount; ++i) {
                        m_indices.push_back(buf[i] + vertexStart);
                    }
                    break;
                }
                default: throw std::runtime_error("unsupported index data type");
                }
            }

            Primitive primitive = {
                m_vao,
                vertexStart,
                static_cast<uint32_t>(vertexCount),
                indexStart,
                static_cast<uint32_t>(indexCount),
                gltfPrimitive.material >= 0 ? m_materials[gltfPrimitive.material + 1].get()
                                            : m_materials[0].get()};
            node->primitives.push_back(primitive);
        }
    }

    if (parent) {
        parent->children.push_back(node.get());
    } else {
        m_rootNodes.push_back(node.get());
    }

    // process child nodes
    for (uint32_t i = 0; i < gltfNode.children_count; ++i) {
        const int32_t childIndex = gltfNode.children[i];
        loadNode(node.get(), gltfModel.nodes[childIndex], childIndex, gltfModel);
    }

    m_nodes.push_back(std::move(node));
}

void Model::createGraphicResources(size_t vertexCount, size_t indexCount) {
    // create a vertex array object
    glGenVertexArrays(1, &m_vao);
    // create a vertex buffer object
    glGenBuffers(1, &m_vbo);
    // create a element array buffer
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCount, NULL, GL_STATIC_DRAW);

    if (indexCount > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), NULL, GL_STATIC_DRAW);
    }

    // specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the
    // attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord0));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord1));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

void Model::updateGraphicResources() {
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * m_vertices.size(), m_vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferSubData(
        GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(uint32_t) * m_indices.size(), m_indices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Model::cleanup() {
    m_nodes.clear();
    m_rootNodes.clear();

    m_materials.clear();
    m_samplers.clear();
    m_textures.clear();

    m_vertices.clear();
    m_indices.clear();

    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_ibo != 0) {
        glDeleteBuffers(1, &m_ibo);
        m_ibo = 0;
    }
}

std::pair<size_t, size_t> Model::getNodeProps(
    const tg3_node& node, const tg3_model& model) {

    size_t vertexCount = 0;
    size_t indexCount = 0;

    if (node.mesh >= 0) {
        const tg3_mesh& mesh = model.meshes[node.mesh];
        for (uint32_t i = 0; i < mesh.primitives_count; ++i) {
            const tg3_primitive& primitive = mesh.primitives[i];
            const tg3_str_int_pair* position = findAttribute(primitive, "POSITION");
            if (!position) throw std::runtime_error("find position data failure");
            vertexCount += model.accessors[position->value].count;
            if (primitive.indices >= 0) {
                indexCount += model.accessors[primitive.indices].count;
            }
        }
    }

    for (uint32_t i = 0; i < node.children_count; ++i) {
        auto result = getNodeProps(model.nodes[node.children[i]], model);
        vertexCount += result.first;
        indexCount += result.second;
    }

    return {vertexCount, indexCount};
}

template <typename T>
bool Model::getAttributeBufferInfo(
    const tg3_model& gltfModel, const tg3_primitive& gltfPrimitive,
    const std::string& name, const T*& data, int& byteStride, size_t& count) {
    const tg3_str_int_pair* attribute = findAttribute(gltfPrimitive, name);
    if (!attribute) return false;

    const tg3_accessor& accessor = gltfModel.accessors[attribute->value];
    if (accessor.buffer_view < 0) return false;
    const tg3_buffer_view& bufferView = gltfModel.buffer_views[accessor.buffer_view];

    data = reinterpret_cast<const T*>(
        gltfModel.buffers[bufferView.buffer].data.data + accessor.byte_offset + bufferView.byte_offset);

    count = accessor.count;

    byteStride = tg3_accessor_byte_stride(&accessor, &bufferView);

    return true;
}

Node* Model::getNodeByIndex(int index) const {
    for (const auto& node : m_nodes) {
        if (node->index == index) {
            return node.get();
        }
    }

    return nullptr;
}

int Model::getSamplerIndex(const Sampler* sampler) const {
    for (size_t i = 0; i < m_samplers.size(); ++i) {
        if (sampler == m_samplers[i].get()) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

int Model::getTextureIndex(const Texture* texture) const {
    for (size_t i = 0; i < m_textures.size(); ++i) {
        if (texture == m_textures[i].get()) {
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
    for (size_t i = 0; i < m_textures.size(); ++i) {
        std::cout << "  + texture[" << i << "]: ";
        ImageTexture2D* tex2D = dynamic_cast<ImageTexture2D*>(m_textures[i].get());
        if (tex2D) {
            std::cout << tex2D->getUri() << std::endl;
        } else {
            std::cout << std::endl;
        }
    }
}

void Model::printMaterials() const {
    std::cout << "+ Materials" << std::endl;
    for (size_t i = 0; i < m_materials.size(); ++i) {
        std::string alphaMode;
        switch (m_materials[i]->alphaMode) {
        case PbrMaterial::AlphaMode::Mask: alphaMode = "Mask"; break;
        case PbrMaterial::AlphaMode::Blend: alphaMode = "Blend"; break;
        case PbrMaterial::AlphaMode::Opaque: alphaMode = "Opaque"; break;
        }

        std::cout << "  + material[" << i << "]"
                  << "\n";
        std::cout << "    + name:        " << m_materials[i]->name << "\n";
        std::cout << "    + doubleSided: " << m_materials[i]->doubleSided << "\n";
        std::cout << "    + alphaMode:   " << alphaMode << "\n";
        std::cout << "    + alphaCutoff: " << m_materials[i]->alphaCutoff << "\n";
        std::cout << "    + albedo: "
                  << "\n";
        std::cout << "      + factor: " << m_materials[i]->albedoFactor << "\n";
        std::cout << "      + texture: " << getTextureIndex(m_materials[i]->albedoMap) << "\n";
        std::cout << "      + sampler: " << getSamplerIndex(m_materials[i]->albeodoSampler) << "\n";
        std::cout << "      + texCoordSet: " << m_materials[i]->texCoordSets.albedo << "\n";
        std::cout << "    + metallic: "
                  << "\n";
        std::cout << "      + factor: " << m_materials[i]->metallicFactor << "\n";
        std::cout << "      + texture: " << getTextureIndex(m_materials[i]->metallicMap) << "\n";
        std::cout << "      + sampler: " << getSamplerIndex(m_materials[i]->metallicSampler) << "\n";
        std::cout << "      + texCoordSet: " << m_materials[i]->texCoordSets.metallic << "\n";
        std::cout << "    + roughness: "
                  << "\n";
        std::cout << "      + factor: " << m_materials[i]->metallicFactor << "\n";
        std::cout << "      + texture: " << getTextureIndex(m_materials[i]->roughnessMap) << "\n";
        std::cout << "      + sampler: " << getSamplerIndex(m_materials[i]->roughnessSampler)
                  << "\n";
        std::cout << "      + texCoordSet: " << m_materials[i]->texCoordSets.roughness << "\n";
        std::cout << "    + normal: "
                  << "\n";
        std::cout << "      + texture: " << getTextureIndex(m_materials[i]->normalMap) << "\n";
        std::cout << "      + sampler: " << getSamplerIndex(m_materials[i]->normalSampler) << "\n";
        std::cout << "      + texCoordSet: " << m_materials[i]->texCoordSets.normal << "\n";
        std::cout << "    + occlusion: "
                  << "\n";
        std::cout << "      + strength:" << m_materials[i]->occlusionStrength << "\n";
        std::cout << "      + texture: " << getTextureIndex(m_materials[i]->occlusionMap) << "\n";
        std::cout << "      + sampler: " << getSamplerIndex(m_materials[i]->occlusionSampler)
                  << "\n";
        std::cout << "      + texCoordSet: " << m_materials[i]->texCoordSets.metallic << "\n";
        std::cout << "    + emissive: "
                  << "\n";
        std::cout << "      + factor: " << m_materials[i]->emissiveFactor << "\n";
        std::cout << "      + texture: " << getTextureIndex(m_materials[i]->emissiveMap) << "\n";
        std::cout << "      + sampler: " << getSamplerIndex(m_materials[i]->emissiveSampler) << "\n";
        std::cout << "      + texCoordSet: " << m_materials[i]->texCoordSets.emissive << "\n";
    }
}

void Model::printNodeHierachy() const {
    std::cout << "+ Node Hierachy" << std::endl;
    for (const Node* node : m_rootNodes) {
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
