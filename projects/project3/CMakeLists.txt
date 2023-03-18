cmake_minimum_required(VERSION 3.10)

project(project3)

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)

set(BASE_HDR ../base/gl_utility.h
             ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/camera.h
             ../base/frustum.h
             ../base/plane.h
             ../base/transform.h
             ../base/model.h
             ../base/bounding_box.h
             ../base/vertex.h)

set(BASE_SRC ../base/application.cpp
             ../base/glsl_program.cpp
             ../base/camera.cpp
             ../base/transform.cpp
             ../base/model.cpp)

add_executable(${PROJECT_NAME} ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR})

source_group("Header Files" FILES ${BASE_HDR} ${PROJECT_HDR})
source_group("Source Files" FILES ${BASE_SRC} ${PROJECT_SRC})

configure_project(${PROJECT_NAME})

if (EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PRIVATE "-fexceptions")
endif()

if (USE_GLES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GLES)
endif()

if (EMSCRIPTEN)
    set(asset_path "obj/bunny.obj")
    preload_files(${PROJECT_NAME} asset_path)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE glm)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader)

if (EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s USE_GLFW=3")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s MIN_WEBGL_VERSION=2")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s MAX_WEBGL_VERSION=2")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s WASM=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s NO_DISABLE_EXCEPTION_CATCHING=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s ALLOW_MEMORY_GROWTH=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s GL_SUPPORT_AUTOMATIC_ENABLE_EXTENSIONS=1")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    target_link_libraries(${PROJECT_NAME} PRIVATE glad)
endif()