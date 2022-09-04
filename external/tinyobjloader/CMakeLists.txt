# Created By Yun Yang
cmake_minimum_required(VERSION 3.10)

project(tinyobjloader)

aux_source_directory(. TINY_OBJ_LOADER_SRC)

add_library(tinyobjloader ${TINY_OBJ_LOADER_SRC})

set_target_properties(tinyobjloader PROPERTIES FOLDER "lib")