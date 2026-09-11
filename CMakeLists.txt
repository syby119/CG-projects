cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "utility")

# language settings
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

project(cg_projects LANGUAGES C CXX VERSION 1.1)

include("cmake/configure_project.cmake")
include("cmake/hardlink_shaders.cmake")

if (EMSCRIPTEN)
    include("cmake/preload_files.cmake")
endif()

# build in source in not recommended
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(WARNING "The binary directory of CMake should not \
                     be the same as source directory")
endif()

# build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# disable max/min definition in windows
if (WIN32)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

# copy media data to the build directory
file(COPY "media/" DESTINATION "media")

# add libraries
if(EMSCRIPTEN)
    message(STATUS "use GLFW in Emscripten")
    message(STATUS "use OpenGL/ES in Emscripten")
else()
    set(GLFW_BUILD_EXAMPLES OFF)
    set(GLFW_BUILD_TESTS OFF)
    set(GLFW_BUILD_DOCS OFF)
    add_subdirectory(./external/glfw)
    set_target_properties(glfw PROPERTIES FOLDER "lib/glfw")
    set_target_properties(uninstall PROPERTIES FOLDER "lib/glfw")
    set_target_properties(update_mappings PROPERTIES FOLDER "lib/glfw")

    add_subdirectory(./external/glad)
    set_target_properties(glad PROPERTIES FOLDER "lib")
endif()

add_subdirectory(./external/glm)
set_target_properties(glm PROPERTIES FOLDER "lib/glm")

# This project uses tinyobjloader's C++ API only.
set(TINYOBJLOADER_BUILD_C_LIBRARY OFF CACHE BOOL
    "Build the pure-C11 loader (tiny_obj_c)" FORCE)
add_subdirectory(./external/tinyobjloader)
set_target_properties(tinyobjloader PROPERTIES FOLDER "lib")
if (MSVC)
    target_compile_options(tinyobjloader PUBLIC /utf-8)
endif()

add_subdirectory(./external/imgui)
set_target_properties(imgui PROPERTIES FOLDER "lib")

add_subdirectory(./external/stb)
set_target_properties(stb PROPERTIES FOLDER "lib")

add_library(tinygltf_v3 STATIC external/tinygltf/tiny_gltf_v3.c)
target_compile_features(tinygltf_v3 PUBLIC c_std_11)
target_compile_definitions(tinygltf_v3 PRIVATE TINYGLTF3_ENABLE_FS)
target_include_directories(tinygltf_v3 PUBLIC external/tinygltf)
if (MSVC)
    target_compile_options(tinygltf_v3 PUBLIC /utf-8)
endif()
set_target_properties(tinygltf_v3 PROPERTIES FOLDER "lib")

add_subdirectory(./external/meshoptimizer)
set_target_properties(meshoptimizer PROPERTIES FOLDER "lib")

add_subdirectory(./external/magic_enum)
set_target_properties(magic_enum PROPERTIES FOLDER "lib")

set(SPIRV_HEADERS_ENABLE_TESTS OFF CACHE INTERNAL "Skip building SPIRV-Headers tests" FORCE)
add_subdirectory(./external/SPIRV-HEADERS)

set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "Skip building SPIRV-Tools executables" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "Skip building SPIRV-Tools tests" FORCE)
add_subdirectory(./external/SPIRV-Tools)
foreach(target IN ITEMS spirv-tools-build-version spirv-tools-header-DebugInfo
        spirv-tools-header-NonSemanticShaderDebugInfo100 spirv-tools-header-OpenCLDebugInfo100
        spv-tools-cldi100 spv-tools-clspvreflection spv-tools-debuginfo spv-tools-shdi100
        spv-tools-spv-amd-gs spv-tools-spv-amd-sb spv-tools-spv-amd-sevp spv-tools-spv-amd-stm
        spv-tools-vkspreflection SPIRV-Tools-diff SPIRV-Tools-link SPIRV-Tools-lint
        SPIRV-Tools-opt SPIRV-Tools-reduce SPIRV-Tools-shared SPIRV-Tools-static
        spirv-tools-vimsyntax core_tables enum_string_mapping extinst_tables spirv-tools-pkg-config
        spirv-tools-check-python-tests spirv-tools-check-python-types spirv-tools-tables)
    if (TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER "lib/spirv-tools")
    endif()
endforeach()

set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Skip building SPIRV-Cross tests" FORCE)
add_subdirectory(./external/SPIRV-Cross)
foreach(target IN ITEMS spirv-cross spirv-cross-c spirv-cross-core spirv-cross-cpp spirv-cross-glsl
        spirv-cross-hlsl spirv-cross-msl spirv-cross-reflect spirv-cross-util)
    if (TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER "lib/spirv-cross")
    endif()
endforeach()

set(GLSLANG_TESTS OFF CACHE BOOL "Skip building glslang tests" FORCE)
add_subdirectory(./external/glslang)
foreach(target IN ITEMS GenericCodeGen glslang glslang-default-resource-limits MachineIndependent
        OSDependent SPIRV SPVRemapper glslang-standalone spirv-remap)
    if (TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER "lib/glslang")
    endif()
endforeach()

set(SHADERC_SKIP_TESTS ON CACHE INTERNAL "Skip building tests" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE INTERNAL "Skip building examples" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE INTERNAL "Skip copyright check" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE INTERNAL "Enable Shaderc Shared CRT" FORCE)
add_subdirectory(./external/shaderc)
foreach(target IN ITEMS add-copyright build-version glslc glslc_exe shaderc shaderc_combined
        shaderc_combined-pkg-config shaderc_shared shaderc_static-pkg-config shaderc_util
        shaderc-pkg-config testdata)
    if (TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER "lib/shaderc")
    endif()
endforeach()

# add projects
set(PROJECTS_DIR ${CMAKE_SOURCE_DIR}/projects)

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if (CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE PROJECTS_SOURCE_FILES CONFIGURE_DEPENDS
         "${PROJECTS_DIR}/*.c"
         "${PROJECTS_DIR}/*.cc"
         "${PROJECTS_DIR}/*.cpp"
         "${PROJECTS_DIR}/*.cxx"
         "${PROJECTS_DIR}/*.h"
         "${PROJECTS_DIR}/*.hh"
         "${PROJECTS_DIR}/*.hpp"
         "${PROJECTS_DIR}/*.hxx")

    add_custom_target(format-projects
        COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i -style=file ${PROJECTS_SOURCE_FILES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Formatting project source files"
        VERBATIM)
    set_target_properties(format-projects PROPERTIES FOLDER "utility")
endif()

file(GLOB targets LIST_DIRECTORIES true
     RELATIVE ${PROJECTS_DIR} "${PROJECTS_DIR}/*")

foreach (target ${targets})
    set(TARGET_ABSOLUTE_PATH ${PROJECTS_DIR}/${target})
    if (NOT ((IS_DIRECTORY ${TARGET_ABSOLUTE_PATH})
            AND (EXISTS "${TARGET_ABSOLUTE_PATH}/CMakeLists.txt")))
        continue()
    endif()
    add_subdirectory(${PROJECTS_DIR}/${target})
    if (${target} MATCHES "project*")
        set_target_properties(${target} PROPERTIES FOLDER "project")
    elseif (${target} MATCHES "bonus")
        set_target_properties(${target} PROPERTIES FOLDER "bonus")
    else()
        set_target_properties(${target} PROPERTIES FOLDER "demo")
    endif()
endforeach()
