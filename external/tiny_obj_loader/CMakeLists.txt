# Created By Yun Yang
cmake_minimum_required(VERSION 3.10)

project(tiny_obj_loader)

aux_source_directory(. TINY_OBJ_LOADER_SRC)

add_library(tiny_obj_loader ${TINY_OBJ_LOADER_SRC})

set_target_properties(tiny_obj_loader PROPERTIES FOLDER "lib")