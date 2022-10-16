cmake_minimum_required(VERSION 3.10)

project(bonus4)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glm)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glad/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glfw/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/tinyobjloader)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/imgui)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/stb)

aux_source_directory(. PROJECT_SRC)

file(GLOB PROJECT_HDR ./*.h)

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
             ../base/fullscreen_quad.h
             ../base/texture.h
             ../base/texture2d.h
             ../base/texture_cubemap.h)

set(BASE_SRC ../base/application.cpp 
             ../base/glsl_program.cpp 
             ../base/camera.cpp 
             ../base/transform.cpp
             ../base/model.cpp
             ../base/texture.cpp
             ../base/texture2d.cpp
             ../base/texture_cubemap.cpp
             ../base/fullscreen_quad.cpp
             ../base/instanced_model.cpp)

add_executable(bonus4 ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR})

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

target_link_libraries(bonus4 glm)

target_link_libraries(bonus4 glad)

target_link_libraries(bonus4 glfw)

target_link_libraries(bonus4 tinyobjloader)

target_link_libraries(bonus4 imgui)

target_link_libraries(bonus4 stb)