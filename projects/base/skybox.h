#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "gl_utility.h"
#include "glsl_program.h"
#include "texture_cubemap.h"

class SkyBox {
public:
    SkyBox(const std::vector<std::string>& textureFilenames);

    SkyBox(SkyBox&& rhs) noexcept;

    ~SkyBox();

    void draw(const glm::mat4& projection, const glm::mat4& view);

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    std::unique_ptr<TextureCubemap> m_texture;

    std::unique_ptr<GLSLProgram> m_shader;

    void cleanup();
};