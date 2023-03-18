cmake_minimum_required(VERSION 3.20)

project(bonus4)

set(SHADER_TARGET_PATH ${CMAKE_BINARY_DIR}/media/shader/${PROJECT_NAME})

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)
file(GLOB PROJECT_SHADERS ./*.vert ./*.geom ./*.frag)

set(BASE_HDR ../base/gl_utility.h
             ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/camera.h
             ../base/frustum.h
             ../base/plane.h
             ../base/transform.h
             ../base/model.h
             ../base/bounding_box.h
             ../base/vertex.h
             ../base/light.h
             ../base/framebuffer.h
             ../base/fullscreen_quad.h
             ../base/texture.h
             ../base/texture2d.h
             ../base/texture_cubemap.h)

set(BASE_SRC ../base/application.cpp
             ../base/glsl_program.cpp
             ../base/camera.cpp
             ../base/transform.cpp
             ../base/model.cpp
             ../base/texture.cpp
             ../base/texture2d.cpp
             ../base/texture_cubemap.cpp
             ../base/framebuffer.cpp
             ../base/fullscreen_quad.cpp)

add_executable(${PROJECT_NAME} ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR} ${PROJECT_SHADERS})

source_group("Header Files" FILES ${BASE_HDR} ${PROJECT_HDR})
source_group("Source Files" FILES ${BASE_SRC} ${PROJECT_SRC})
source_group("Shader Files" FILES ${PROJECT_SHADERS})

hardlink_shaders(${PROJECT_NAME} ${SHADER_TARGET_PATH} PROJECT_SHADERS)

configure_project(${PROJECT_NAME})

if (EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PRIVATE "-fexceptions")
endif()

if (USE_GLES)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_GLES)
endif()

if (EMSCRIPTEN)
    set(asset_paths
        "obj/bunny.obj"
        "obj/arrow.obj"
        "obj/sphere.obj"
        "obj/cube.obj")
    foreach(shader ${PROJECT_SHADERS})
        get_filename_component(shader_name ${shader} NAME)
        list(APPEND asset_paths "shader/${PROJECT_NAME}/${shader_name}")
    endforeach()

    preload_files(${PROJECT_NAME} asset_paths)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE glm)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_link_libraries(${PROJECT_NAME} PRIVATE stb)

if (EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s USE_GLFW=3")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s MIN_WEBGL_VERSION=2")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s MAX_WEBGL_VERSION=2")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s WASM=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s NO_DISABLE_EXCEPTION_CATCHING=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s ALLOW_MEMORY_GROWTH=1")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-s GL_SUPPORT_AUTOMATIC_ENABLE_EXTENSIONS=1")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
    target_link_libraries(${PROJECT_NAME} PRIVATE glad)
endif()