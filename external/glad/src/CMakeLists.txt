cmake_minimum_required(VERSION 3.10)

aux_source_directory(. DIR_LIB_SRCS)

include_directories(../include)

add_library(glad ${DIR_LIB_SRCS})

set_target_properties(glad PROPERTIES FOLDER "lib")