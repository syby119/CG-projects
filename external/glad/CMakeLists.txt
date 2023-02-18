cmake_minimum_required(VERSION 3.15)

project(glad VERSION 2.0)

if (USE_GLES)
    add_library(glad
        ./include/glad/gles2.h 
        ./include/KHR/khrplatform.h 
        ./src/gles2.c
    )
else()
    add_library(glad
        ./include/glad/gl.h 
        ./include/KHR/khrplatform.h 
        ./src/gl.c
    )
endif()

target_include_directories(glad PUBLIC ./include)