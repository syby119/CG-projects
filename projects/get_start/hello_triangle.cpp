#include "hello_triangle.h"
#include <iostream>

HelloTriangle::HelloTriangle(const Options& options) : Application(options) {
    // create a vertex array object
    glGenVertexArrays(1, &m_vao);
    // create a vertex buffer object
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_STATIC_DRAW);

    // specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the
    // attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // create shader
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    const char* vsCode =
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec3 aColor;\n"
        "out vec3 color;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0f);\n"
        "    color = aColor;\n"
        "}\n";

    const char* fsCode =
        "in vec3 color;\n"
        "out vec4 outColor;\n"
        "void main() {\n"
        "    outColor = vec4(color, 1.0f);\n"
        "}\n";

    m_shader.reset(new GLSLProgram());
    m_shader->attachVertexShader(vsCode, version);
    m_shader->attachFragmentShader(fsCode, version);

    m_shader->link();
}

HelloTriangle::~HelloTriangle() {
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void HelloTriangle::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }
}

void HelloTriangle::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shader->use();
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}