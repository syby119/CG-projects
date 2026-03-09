cmake_minimum_required(VERSION 3.15)

project(stb)

file(GLOB STB_HDR *.h)
file(GLOB STB_SRC *.c*)

add_library(stb ${STB_HDR} ${STB_SRC})
target_compile_options(stb PRIVATE
    $<$<C_COMPILER_ID:Clang>:-Wno-tautological-compare>
    $<$<C_COMPILER_ID:GNU>:-Wno-tautological-compare>
)

target_include_directories(stb PUBLIC .)