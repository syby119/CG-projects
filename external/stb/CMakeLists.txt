cmake_minimum_required(VERSION 3.15)

project(stb)

file(GLOB STB_HDR *.h)
file(GLOB STB_SRC *.c*)

add_library(stb ${STB_HDR} ${STB_SRC})

target_include_directories(stb INTERFACE .)