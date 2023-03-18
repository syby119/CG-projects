#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <stb_image.h>

#include "../base/framebuffer.h"
#include "../base/glsl_program.h"
#include "../base/sampler.h"
#include "../base/texture.h"

#include "skybox.h"

Skybox::Skybox(
    const std::string& equirectImagePath, const std::string& equirectToCubemapVsFilepath,
    const std::string& equirectToCubemapFsFilepath, const uint32_t resolution) {
    createVertexResources();
    equirectangulerToCubemap(
        equirectImagePath, equirectToCubemapVsFilepath, equirectToCubemapFsFilepath, resolution);
}

Skybox::Skybox(Skybox&& rhs) noexcept
    : _vao(rhs._vao), _vbo(rhs._vbo), _texture(rhs._texture),
      irradianceMap(std::move(rhs.irradianceMap)), prefilterMap(std::move(rhs.prefilterMap)),
      brdfLutMap(std::move(rhs.brdfLutMap)) {
    rhs._vao = 0;
    rhs._vbo = 0;
    rhs._texture = 0;
}

Skybox::~Skybox() {
    cleanup();
}

void Skybox::bindEnvironmentMap(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);
}

uint32_t Skybox::getMaxPrefilterMipLevel() const {
    return _maxPrefilteredMipLevel;
}

void Skybox::draw() const {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void Skybox::createVertexResources() {
    GLfloat vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f, // bottom-left
        1.0f, 1.0f, -1.0f,   // top-right
        1.0f, -1.0f, -1.0f,  // bottom-right
        1.0f, 1.0f, -1.0f,   // top-right
        -1.0f, -1.0f, -1.0f, // bottom-left
        -1.0f, 1.0f, -1.0f,  // top-left
        // front face
        -1.0f, -1.0f, 1.0f, // bottom-left
        1.0f, -1.0f, 1.0f,  // bottom-right
        1.0f, 1.0f, 1.0f,   // top-right
        1.0f, 1.0f, 1.0f,   // top-right
        -1.0f, 1.0f, 1.0f,  // top-left
        -1.0f, -1.0f, 1.0f, // bottom-left
        // left face
        -1.0f, 1.0f, 1.0f,   // top-right
        -1.0f, 1.0f, -1.0f,  // top-left
        -1.0f, -1.0f, -1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, // bottom-left
        -1.0f, -1.0f, 1.0f,  // bottom-right
        -1.0f, 1.0f, 1.0f,   // top-right
                             // right face
        1.0f, 1.0f, 1.0f,    // top-left
        1.0f, -1.0f, -1.0f,  // bottom-right
        1.0f, 1.0f, -1.0f,   // top-right
        1.0f, -1.0f, -1.0f,  // bottom-right
        1.0f, 1.0f, 1.0f,    // top-left
        1.0f, -1.0f, 1.0f,   // bottom-left
        // bottom face
        -1.0f, -1.0f, -1.0f, // top-right
        1.0f, -1.0f, -1.0f,  // top-left
        1.0f, -1.0f, 1.0f,   // bottom-left
        1.0f, -1.0f, 1.0f,   // bottom-left
        -1.0f, -1.0f, 1.0f,  // bottom-right
        -1.0f, -1.0f, -1.0f, // top-right
        // top face
        -1.0f, 1.0f, -1.0f, // top-left
        1.0f, 1.0f, 1.0f,   // bottom-right
        1.0f, 1.0f, -1.0f,  // top-right
        1.0f, 1.0f, 1.0f,   // bottom-right
        -1.0f, 1.0f, -1.0f, // top-left
        -1.0f, 1.0f, 1.0f   // bottom-left
    };

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glBindVertexArray(0);
}

void Skybox::equirectangulerToCubemap(
    const std::string& equirectImagePath, const std::string& equirectToCubemapVsFilepath,
    const std::string& equirectToCubemapFsFilepath, uint32_t resolution) {
    resolution = nextPow2(resolution);

    // create cubemap texture
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);
    for (uint32_t i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, resolution, resolution, 0, GL_RGB,
            GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // load image and create texture
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    std::unique_ptr<float, void (*)(float*)> data(
        stbi_loadf(equirectImagePath.c_str(), &width, &height, &channels, 0), [](float* data) {
            stbi_image_free(data);
        });

    if (data == nullptr) {
        throw std::runtime_error("load " + equirectImagePath + " failure");
    }

    ImageTexture2D hdrTexture(
        data.get(), width, height, channels, GL_RGB16F, GL_RGB, GL_FLOAT, equirectImagePath);

    // create sampler for equirect texture
    Sampler hdrSampler;
    hdrSampler.setInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    hdrSampler.setInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    hdrSampler.setInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    hdrSampler.setInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // create mapping shader
    GLSLProgram shader;
    shader.attachVertexShaderFromFile(equirectToCubemapVsFilepath);
    shader.attachFragmentShaderFromFile(equirectToCubemapFsFilepath);
    shader.link();

    const glm::mat4 projection = getProjection();
    const std::array<glm::mat4, 6> views = getViews();

    shader.use();
    shader.setUniformMat4("projection", projection);
    shader.setUniformInt("equirectangularMap", 0);
    hdrTexture.bind(0);
    hdrSampler.bind(0);

    // remember previous viewport
    glm::ivec4 viewport;
    glGetIntegerv(GL_VIEWPORT, &viewport[0]);

    // create framebuffer
    Framebuffer framebuffer;
    framebuffer.bind();

    glViewport(0, 0, resolution, resolution);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);

    for (uint32_t i = 0; i < 6; ++i) {
        shader.setUniformMat4("view", views[i]);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, _texture, 0);

        GLenum status = framebuffer.checkStatus();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(
                "convert equirectanguler map to cubemap failure, "
                + framebuffer.getDiagnostic(status));
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw();
    }

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    framebuffer.unbind();

    // restore viewport
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    hdrTexture.unbind();
    hdrSampler.unbind(0);

    // generate mipmap
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Skybox::cleanup() {
    if (_vao != 0) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }

    if (_vbo != 0) {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
        _texture = 0;
    }
}

void Skybox::generateIrradianceMap(
    const std::string& irradianceConvolutionVsFilepath,
    const std::string& irradianceConvolutionFsFilepath, uint32_t resolution, float deltaTheta,
    float deltaPhi) {
    resolution = nextPow2(resolution);

    // create irradianceMap texture
    irradianceMap.reset(new TextureCubemap(GL_RGB16F, resolution, resolution, GL_RGB, GL_FLOAT));

    irradianceMap->bind();
    irradianceMap->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    irradianceMap->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    irradianceMap->setParamterInt(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    irradianceMap->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    irradianceMap->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    irradianceMap->unbind();

    // create irradiance shader
    GLSLProgram shader;
    shader.attachVertexShaderFromFile(irradianceConvolutionVsFilepath);
    shader.attachFragmentShaderFromFile(irradianceConvolutionFsFilepath);
    shader.link();

    const glm::mat4 projection = getProjection();
    const std::array<glm::mat4, 6> views = getViews();

    shader.use();
    shader.setUniformMat4("projection", projection);
    shader.setUniformInt("environmentMap", 0);
    shader.setUniformFloat("deltaTheta", deltaTheta);
    shader.setUniformFloat("deltaPhi", deltaPhi);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);

    // remember previous viewport
    glm::ivec4 viewport;
    glGetIntegerv(GL_VIEWPORT, &viewport[0]);

    // use the framebuffer to render to cubemap
    Framebuffer framebuffer;
    framebuffer.bind();
    glViewport(0, 0, resolution, resolution);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    for (uint32_t i = 0; i < 6; ++i) {
        shader.setUniformMat4("view", views[i]);
        framebuffer.attachTexture2D(
            *irradianceMap, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);

        GLenum status = framebuffer.checkStatus();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(
                "generate irradiance map failure, " + framebuffer.getDiagnostic(status));
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw();
    }
    framebuffer.unbind();

    // restore OpenGL states
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void Skybox::generatePrefilterMap(
    const std::string& prefilterVsFilepath, const std::string& prefilterFsFilepath,
    uint32_t resolution, uint32_t numSamples) {
    constexpr uint32_t maxMipLevels = 5;
    resolution = std::max(nextPow2(resolution), 1u << maxMipLevels);

    // create prefilterMap texture
    prefilterMap.reset(new TextureCubemap(GL_RGB16F, resolution, resolution, GL_RGB, GL_FLOAT));

    prefilterMap->bind();
    prefilterMap->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    prefilterMap->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    prefilterMap->setParamterInt(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    prefilterMap->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    prefilterMap->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // generate mipmap so OpenGL automatically allocates the required memory
    prefilterMap->generateMipmap();
    prefilterMap->unbind();

    // create irradiance shader
    GLSLProgram shader;
    shader.attachVertexShaderFromFile(prefilterVsFilepath);
    shader.attachFragmentShaderFromFile(prefilterFsFilepath);
    shader.link();

    const glm::mat4 projection = getProjection();
    const std::array<glm::mat4, 6> views = getViews();

    shader.use();
    shader.setUniformInt("environmentMap", 0);
    shader.setUniformMat4("projection", projection);
    shader.setUniformUint("numSamples", numSamples);

    // remember previous viewport
    glm::ivec4 viewport;
    glGetIntegerv(GL_VIEWPORT, &viewport[0]);

    // create framebuffer
    Framebuffer framebuffer;
    framebuffer.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _texture);

    uint32_t mipResolution = resolution;
    for (uint32_t mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        // set roughness
        float roughness = static_cast<float>(mipLevel) / (maxMipLevels - 1);
        shader.setUniformFloat("roughness", roughness);

        // fit viewport to mipResolution
        glViewport(0, 0, mipResolution, mipResolution);

        // render prefilter result to mipmap
        for (uint32_t i = 0; i < 6; ++i) {
            shader.setUniformMat4("view", views[i]);
            framebuffer.attachTexture2D(
                *prefilterMap, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mipLevel);

            GLenum status = framebuffer.checkStatus();
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                throw std::runtime_error(
                    "generate prefilter map failure, " + framebuffer.getDiagnostic(status));
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            draw();
        }

        mipResolution >>= 1;
    }

    framebuffer.unbind();

    // restore OpenGL states
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // set prefilteredMipLevels;
    _maxPrefilteredMipLevel = maxMipLevels - 1;
}

void Skybox::generateBrdfLutMap(
    const std::string brdfLutVsFilepath, const std::string brdfLutFsFilepath, uint32_t resolution,
    uint32_t numSamples) {
    resolution = nextPow2(resolution);

    // create brdf look up table texture
    brdfLutMap.reset(new Texture2D(GL_RG16F, resolution, resolution, GL_RG, GL_FLOAT));

    brdfLutMap->bind();
    brdfLutMap->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    brdfLutMap->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    brdfLutMap->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    brdfLutMap->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    brdfLutMap->unbind();

    // create brdf look up table shader
    GLSLProgram shader;
    shader.attachVertexShaderFromFile(brdfLutVsFilepath);
    shader.attachFragmentShaderFromFile(brdfLutFsFilepath);
    shader.link();

    // create a empty vao for rendering
    // https://forums.developer.nvidia.com/t/30167
    GLuint emptyVao = 0;
    glGenVertexArrays(1, &emptyVao);

    // remember previous viewport
    glm::ivec4 viewport;
    glGetIntegerv(GL_VIEWPORT, &viewport[0]);

    Framebuffer framebuffer;
    framebuffer.bind();
    framebuffer.attachTexture2D(*brdfLutMap, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
    GLenum status = framebuffer.checkStatus();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(
            "generate brdf lookup table failure, " + framebuffer.getDiagnostic(status));
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, resolution, resolution);

    shader.use();
    shader.setUniformUint("numSamples", numSamples);
    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    framebuffer.unbind();

    glDeleteVertexArrays(1, &emptyVao);

    // restore OpenGL states
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

glm::mat4 Skybox::getProjection() {
    return glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
}

std::array<glm::mat4, 6> Skybox::getViews() {
    return std::array<glm::mat4, 6>{
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};
}

uint32_t Skybox::nextPow2(uint32_t n) {
    if (n & (n - 1)) {
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return n + 1;
    } else {
        return n == 0 ? 1 : n;
    }
}