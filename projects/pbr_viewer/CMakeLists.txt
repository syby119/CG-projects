cmake_minimum_required(VERSION 3.10)

project(pbr_viewer)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glm)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glad/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glfw/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/tinygltf)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/imgui)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/stb)

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)

set(BASE_HDR ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/camera.h
             ../base/frustum.h
             ../base/plane.h
             ../base/transform.h
             ../base/bounding_box.h
             ../base/light.h
             ../base/sampler.h
             ../base/texture.h
             ../base/fullscreen_quad.h)

set(BASE_SRC ../base/application.cpp 
             ../base/glsl_program.cpp 
             ../base/camera.cpp 
             ../base/transform.cpp
             ../base/texture.cpp
             ../base/fullscreen_quad.cpp)

add_executable(pbr_viewer ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR})

if(WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
elseif(UNIX)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Debug")
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release")
endif()
endif()

target_link_libraries(pbr_viewer glm)

target_link_libraries(pbr_viewer glad)

target_link_libraries(pbr_viewer glfw)

target_link_libraries(pbr_viewer tinygltf)

target_link_libraries(pbr_viewer imgui)

target_link_libraries(pbr_viewer stb)