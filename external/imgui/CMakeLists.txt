# Created By Yun Yang
cmake_minimum_required(VERSION 3.10)

project(imgui)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glfw/include)
include_directories(${THIRD_PARTY_LIBRARY_PATH}/glad/include)

aux_source_directory(. IMGUI_SRC)

add_library(imgui ${IMGUI_SRC})

set_target_properties(imgui PROPERTIES FOLDER "lib")