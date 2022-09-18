cmake_minimum_required(VERSION 3.10)

project(project3)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glm)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glad/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glfw/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/tinyobjloader)

aux_source_directory(. PROJECT_SRC)

set(PROJECT_HDR ./scene_roaming.h)

set(BASE_HDR ../base/application.h
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

add_executable(project3 ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR})

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

target_link_libraries(project3 glm)

target_link_libraries(project3 glad)

target_link_libraries(project3 glfw)

target_link_libraries(project3 tinyobjloader)