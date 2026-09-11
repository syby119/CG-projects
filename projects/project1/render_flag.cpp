#include "render_flag.h"

RenderFlag::RenderFlag(const Options& options) : Application(options) {
    // create star shader
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    const char* vsCode =
        "layout(location = 0) in vec2 aPosition;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPosition, 0.0f, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = vec4(1.0f, 0.870f, 0.0f, 1.0f);\n"
        "}\n";

    m_starShader.reset(new GLSLProgram);
    m_starShader->attachVertexShader(vsCode, version);
    m_starShader->attachFragmentShader(fsCode, version);
    m_starShader->link();

    // TODO: create 5 stars
    // hint: aspect_of_the_window = m_windowWidth / m_windowHeight
    // write your code here
    // ---------------------------------------------------------------
    // m_stars[i].reset(new Star(ndc_position, rotation_in_radians, size_of_star,
    // aspect_of_the_window));
    // ---------------------------------------------------------------
}

void RenderFlag::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }
}

void RenderFlag::renderFrame() {
    showFpsInWindowTitle();

    // we use background as the flag
    glClearColor(0.87f, 0.161f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    m_starShader->use();
    for (int i = 0; i < 5; ++i) {
        if (m_stars[i] != nullptr) {
            m_stars[i]->draw();
        }
    }
}