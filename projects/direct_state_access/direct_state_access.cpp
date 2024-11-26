#include "direct_state_access.h"
#include <array>
#include <stb_image.h>

DirectStateAccess::DirectStateAccess(const Options& options)
    : Application(options) {
    initFramebuffer();
    initGeometry();
    initTexture();
    initProgram();
}

DirectStateAccess::~DirectStateAccess() {
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vertexVbo);
    glDeleteBuffers(1, &_instanceVbo);
    glDeleteBuffers(1, &_ibo);

    glDeleteTextures(1, &_texture);

    glDeleteProgram(_program);
}

void DirectStateAccess::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }
}

void DirectStateAccess::renderFrame() {
    showFpsInWindowTitle();

    // In render, we have to bind the OpenGL objects to state machine to make things work.
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glProgramUniform1i(_program, glGetUniformLocation(_program, "colorTexture"), 0);
    glBindTextureUnit(0, _texture);

    glUseProgram(_program);

    glBindVertexArray(_vao);
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr, 4);
    glBindVertexArray(0);

    glUseProgram(0);
    glBindTextureUnit(0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBlitNamedFramebuffer(_fbo, 0, 0, 0, _windowWidth, _windowHeight,
        0, 0, _windowWidth, _windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectStateAccess::initFramebuffer() {
    // create framebuffer
    glCreateFramebuffers(1, &_fbo);

    // color attachment
    glCreateTextures(GL_TEXTURE_2D, 1, &_colorAttachment);
    glTextureStorage2D(_colorAttachment, 1, GL_RGBA8, _windowWidth, _windowHeight);

    // depth attachment
    glCreateRenderbuffers(1, &_depthAttachment);
    glNamedRenderbufferStorage(_depthAttachment, GL_DEPTH_COMPONENT24, _windowWidth, _windowHeight);

    // attach
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorAttachment, 0);
    glNamedFramebufferRenderbuffer(_fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthAttachment);

    // check
    if (glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("framebuffer is not complete");
    }
}

void DirectStateAccess::initGeometry() {
    std::array vertices{
    //      position            uv
        -0.35f, -0.35f,     0.0f, 0.0f,
        -0.35f, +0.35f,     0.0f, 1.0f,
        +0.35f, -0.35f,     1.0f, 0.0f,
        +0.35f, +0.35f,     1.0f, 1.0f,
    };

    std::array offsets{
    //    x     y
        -0.38f, -0.4f,
        -0.38f, +0.4f,
        +0.38f, -0.4f,
        +0.38f, +0.4f,
    };

    std::array<uint8_t, 6> indices{
        0, 1, 2, 1, 2, 3
    };

    // layout(location = xxx)
    constexpr uint32_t positionLocatioin{ 0 };
    constexpr uint32_t texCoordLocatioin{ 1 };
    constexpr uint32_t offsetLocatioin{ 2 };

    // as we only upload the data once, and will not read/write it later,
    // the flags can be set to 0. Possible values can be the combination of
    // + GL_MAP_READ_BIT 
    // + GL_MAP_WRITE_BIT 
    // + GL_MAP_PERSISTENT_BIT 
    // + GL_MAP_COHERENT_BIT
    // +_GL_DYNAMIC_STORAGE_BIT
    // + GL_CLIENT_STORAGE_BIT
    GLbitfield constexpr flags{ 0 };

    // vbos
    glCreateBuffers(1, &_vertexVbo);
    glNamedBufferStorage(_vertexVbo,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), flags);

    glCreateBuffers(1, &_instanceVbo);
    glNamedBufferStorage(_instanceVbo,
        static_cast<GLsizeiptr>(offsets.size() * sizeof(float)), offsets.data(), flags);

    // ibo
    glCreateBuffers(1, &_ibo);
    glNamedBufferStorage(_ibo,
        static_cast<GLsizeiptr>(indices.size() * sizeof(uint8_t)), indices.data(), flags);

    // vao
    glCreateVertexArrays(1, &_vao);

    glVertexArrayVertexBuffer(_vao, 0, _vertexVbo, 0, 4 * sizeof(float));
    glVertexArrayVertexBuffer(_vao, 1, _instanceVbo, 0, 2 * sizeof(float));
    glVertexArrayElementBuffer(_vao, _ibo);

    glEnableVertexArrayAttrib(_vao, positionLocatioin);
    glVertexArrayAttribFormat(_vao, positionLocatioin, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(_vao, positionLocatioin, 0);

    glEnableVertexArrayAttrib(_vao, texCoordLocatioin);
    glVertexArrayAttribFormat(_vao, texCoordLocatioin, 2, GL_FLOAT, GL_FALSE, uint32_t(2 * sizeof(float)));
    glVertexArrayAttribBinding(_vao, texCoordLocatioin, 0);

    glEnableVertexArrayAttrib(_vao, offsetLocatioin);
    glVertexArrayAttribFormat(_vao, offsetLocatioin, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(_vao, offsetLocatioin, 1);
    glVertexArrayBindingDivisor(_vao, 1, 1);
}

void DirectStateAccess::initTexture() {
    std::string path = getAssetFullPath("texture/miscellaneous/dolphin.png");

    stbi_set_flip_vertically_on_load(true);
    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        throw std::runtime_error("load " + path + " failure");
    }

    // choose image format
    GLenum format = GL_RGB;
    switch (channels) {
    case 1: format = GL_RED; break;
    case 3: format = GL_RGB; break;
    case 4: format = GL_RGBA; break;
    default:
        stbi_image_free(data);
        throw std::runtime_error("unsupported format");
    }

    // internal format must be sized format rather than general format
    // https://docs.gl/gl4/glTexStorage2D
    GLint internalFormat = GL_RGB8;
    switch (channels) {
    case 1: internalFormat = GL_R8; break;
    case 3: internalFormat = GL_RGB8; break;
    case 4: internalFormat = GL_RGBA8; break;
    default:
        stbi_image_free(data);
        throw std::runtime_error("unsupported internal format");
    }

    // texture
    glCreateTextures(GL_TEXTURE_2D, 1, &_texture);

    glTextureStorage2D(_texture, 1, internalFormat, width, height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTextureSubImage2D(_texture, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTextureParameteri(_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(_texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(_texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
}

void DirectStateAccess::initProgram() {
    const char* vsCode = R"(
        #version 330 core
        
        layout(location = 0) in vec2 position;
        layout(location = 1) in vec2 uv;
        layout(location = 2) in vec2 offset;

        out vec2 texCoord;

        void main() {
            texCoord = uv;
            gl_Position = vec4(position + offset, 0.0f, 1.0f);
            //gl_Position = vec4(position, 0.0f, 1.0f);
        }
    )";

    const char* fsCode = R"(
        #version 330 core

        layout(location = 0) out vec4 fragColor;

        in vec2 texCoord;
        uniform sampler2D colorTexture;

        void main() {
            fragColor = texture(colorTexture, texCoord);
        }
    )";

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vsCode, nullptr);
    glCompileShader(vertShader);

    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fsCode, nullptr);
    glCompileShader(fragShader);

    _program = glCreateProgram();
    glAttachShader(_program, vertShader);
    glAttachShader(_program, fragShader);

    glLinkProgram(_program);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}