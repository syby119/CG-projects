#include "render_flag.h"

RenderFlag::RenderFlag(const Options& options): Application(options) {
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

    _starShader.reset(new GLSLProgram);
    _starShader->attachVertexShader(vsCode, version);
    _starShader->attachFragmentShader(fsCode, version);
    _starShader->link();

    // TODO: create 5 stars
    // hint: aspect_of_the_window = _windowWidth / _windowHeight
    // write your code here
    // ---------------------------------------------------------------
    // _stars[i].reset(new Star(ndc_position, rotation_in_radians, size_of_star, aspect_of_the_window));
    // ---------------------------------------------------------------
}

void RenderFlag::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return ;
    }
}

void RenderFlag::renderFrame() {
    showFpsInWindowTitle();

    // we use background as the flag
    glClearColor(0.87f, 0.161f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    _starShader->use();
    for (int i = 0; i < 5; ++i) {
        if (_stars[i] != nullptr) {
            _stars[i]->draw();
        }
    }
}