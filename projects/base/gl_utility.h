#pragma once

#include <iostream>
#include <string>

#if defined(__EMSCRIPTEN__)
    #include <webgl/webgl2.h>
#elif defined(USE_GLES)
    #include <glad/gles2.h>
#else
    #include <glad/gl.h>
#endif

inline GLenum implCheckGLErrors(const char* file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
#ifndef __EMSCRIPTEN__
        case GL_STACK_OVERFLOW: error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW: error = "STACK_UNDERFLOW"; break;
#endif
        case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        default: error = "UNKNOWN_ERROR"; break;
        }

        std::cerr << error << " | " << file << " (" << line << ")" << std::endl;
    }

    return errorCode;
}

#define checkGLErrors() implCheckGLErrors(__FILE__, __LINE__)