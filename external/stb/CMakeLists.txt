# Created By Yun Yang
cmake_minimum_required(VERSION 3.10)

project(stb)

aux_source_directory(. STB_SRC)

add_library(stb ${STB_SRC})

set_target_properties(stb PROPERTIES FOLDER "lib")