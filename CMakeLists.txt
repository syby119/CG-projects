cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "utility")

# language settings
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
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

add_subdirectory(./external/tinyobjloader)
set_target_properties(tinyobjloader PROPERTIES FOLDER "lib")

add_subdirectory(./external/imgui)
set_target_properties(imgui PROPERTIES FOLDER "lib")

add_subdirectory(./external/stb)
set_target_properties(stb PROPERTIES FOLDER "lib")

set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_HEADER_ONLY OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(./external/tinygltf)
set_target_properties(tinygltf PROPERTIES FOLDER "lib")

# add projects
set(PROJECTS_DIR ${CMAKE_SOURCE_DIR}/projects)
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