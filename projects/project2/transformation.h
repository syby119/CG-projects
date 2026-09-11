#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../base/application.h"
#include "../base/glsl_program.h"
#include "bunny.h"

class Transformation : public Application {
public:
    Transformation(const Options& options);

    ~Transformation() = default;

    void handleInput() override;

    void renderFrame() override;

private:
    std::vector<Bunny> m_bunnies;

    glm::vec3 m_positions[3] = {
        glm::vec3(-10.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 0.0f, 0.0f)};

    glm::vec3 m_rotateAxis[3] = {
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)};

    float m_rotateAngles[3] = {0.0f, 0.0f, 0.0f};

    glm::vec3 m_scales[3] = {
        glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f)};

    std::unique_ptr<GLSLProgram> m_shader;

    void initShader();
};