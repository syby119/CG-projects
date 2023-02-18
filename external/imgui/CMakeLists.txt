cmake_minimum_required(VERSION 3.10)

project(imgui)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

file(GLOB IMGUI_HDR *.h)
file(GLOB IMGUI_SRC *.cpp)

add_library(imgui ${IMGUI_HDR} ${IMGUI_SRC})

target_include_directories(imgui INTERFACE .)
target_include_directories(imgui PRIVATE ${THIRD_PARTY_LIBRARY_PATH}/glfw/include)
target_include_directories(imgui PRIVATE ${THIRD_PARTY_LIBRARY_PATH}/glad/include)