cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(cg_projects VERSION 1.0)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 14)

file(COPY "media/" DESTINATION "media")

add_subdirectory(./external/glad)
set_target_properties(glad PROPERTIES FOLDER "lib")

add_subdirectory(./external/glm)

set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "utility")

set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
add_subdirectory(./external/glfw)
set_target_properties(glfw PROPERTIES FOLDER "lib/glfw")
set_target_properties(uninstall PROPERTIES FOLDER "lib/glfw")

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

add_subdirectory(./projects/get_start)
set_target_properties(get_start PROPERTIES FOLDER "demo")

add_subdirectory(./projects/pbr_viewer)
set_target_properties(pbr_viewer PROPERTIES FOLDER "demo")

add_subdirectory(./projects/project1)
set_target_properties(project1 PROPERTIES FOLDER "project")

add_subdirectory(./projects/project2)
set_target_properties(project2 PROPERTIES FOLDER "project")

add_subdirectory(./projects/project3)
set_target_properties(project3 PROPERTIES FOLDER "project")

add_subdirectory(./projects/project4)
set_target_properties(project4 PROPERTIES FOLDER "project")

add_subdirectory(./projects/project5)
set_target_properties(project5 PROPERTIES FOLDER "project")

add_subdirectory(./projects/project6)
set_target_properties(project6 PROPERTIES FOLDER "project")

add_subdirectory(./projects/bonus1)
set_target_properties(bonus1 PROPERTIES FOLDER "bonus")

add_subdirectory(./projects/bonus2)
set_target_properties(bonus2 PROPERTIES FOLDER "bonus")

add_subdirectory(./projects/bonus3)
set_target_properties(bonus3 PROPERTIES FOLDER "bonus")

add_subdirectory(./projects/bonus4)
set_target_properties(bonus4 PROPERTIES FOLDER "bonus")

add_subdirectory(./projects/bonus5)
set_target_properties(bonus5 PROPERTIES FOLDER "bonus")

