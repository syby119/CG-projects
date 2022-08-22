cmake_minimum_required(VERSION 3.10)

project(get_start)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glm)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glad/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glfw/include)

aux_source_directory(. PROJECT_SRC)

set(PROJECT_HDR ./hello_triangle.h)

set(BASE_HDR ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h)

set(BASE_SRC ../base/application.cpp ../base/glsl_program.cpp)

add_executable(get_start ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR})

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

target_link_libraries(get_start glm)

target_link_libraries(get_start glad)

target_link_libraries(get_start glfw)