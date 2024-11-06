cmake_minimum_required(VERSION 3.20)

project(mesh_shading_pipeline)

set(SHADER_TARGET_PATH ${CMAKE_BINARY_DIR}/media/shader/${PROJECT_NAME})

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)
file(GLOB PROJECT_SHADERS ./*.vert ./*.frag ./*.task ./*.mesh)

set(BASE_HDR ../base/gl_utility.h
             ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/transform.h
             ../base/camera.h
             ../base/bounding_box.h
             ../base/texture.h
             ../base/bounding_box.h
             ../base/model.h)

set(BASE_SRC ../base/application.cpp
             ../base/glsl_program.cpp
             ../base/transform.cpp
             ../base/camera.cpp
             ../base/model.cpp)

add_executable(${PROJECT_NAME} ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR} ${PROJECT_SHADERS})

source_group("Header Files" FILES ${BASE_HDR} ${PROJECT_HDR})
source_group("Source Files" FILES ${BASE_SRC} ${PROJECT_SRC})
source_group("Shader Files" FILES ${PROJECT_SHADERS})

hardlink_shaders(${PROJECT_NAME} ${SHADER_TARGET_PATH} PROJECT_SHADERS)

configure_project(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)
target_link_libraries(${PROJECT_NAME} PRIVATE glm)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE stb)
target_link_libraries(${PROJECT_NAME} PRIVATE meshoptimizer)