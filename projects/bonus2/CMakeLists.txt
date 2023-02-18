cmake_minimum_required(VERSION 3.20)

project(bonus2)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)
set(SHADER_TARGET_PATH ${CMAKE_BINARY_DIR}/media/shader/bonus2)

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)
file(GLOB PROJECT_SHADERS ./*.vert ./*.geom ./*.frag)

set(BASE_HDR ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/camera.h
             ../base/frustum.h
             ../base/plane.h
             ../base/transform.h
             ../base/model.h
             ../base/instanced_model.h
             ../base/bounding_box.h
             ../base/vertex.h
             ../base/light.h
             ../base/texture.h
             ../base/texture2d.h)

set(BASE_SRC ../base/application.cpp 
             ../base/glsl_program.cpp 
             ../base/camera.cpp 
             ../base/transform.cpp
             ../base/model.cpp
             ../base/texture.cpp
             ../base/texture2d.cpp
             ../base/instanced_model.cpp)

add_executable(bonus2 ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR} ${PROJECT_SHADERS})

source_group("Shader Files" FILES ${PROJECT_SHADERS})

hardlink_shaders(${PROJECT_NAME} ${SHADER_TARGET_PATH} PROJECT_SHADERS)

configure_project(${PROJECT_NAME})

target_include_directories(bonus2 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glm)
target_include_directories(bonus2 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glad/include)
target_include_directories(bonus2 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glfw/include)
target_include_directories(bonus2 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/tinyobjloader)
target_include_directories(bonus2 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/imgui)
target_include_directories(bonus2 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/stb)

target_link_libraries(bonus2 glm)
target_link_libraries(bonus2 glad)
target_link_libraries(bonus2 glfw)
target_link_libraries(bonus2 tinyobjloader)
target_link_libraries(bonus2 imgui)
target_link_libraries(bonus2 stb)