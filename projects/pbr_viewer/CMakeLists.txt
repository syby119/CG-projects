cmake_minimum_required(VERSION 3.20)

project(pbr_viewer)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)
set(SHADER_TARGET_PATH ${CMAKE_BINARY_DIR}/media/shader/pbr_viewer)

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)
file(GLOB PROJECT_SHADERS ./*.vert ./*.geom ./*.frag)

set(BASE_HDR ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/camera.h
             ../base/frustum.h
             ../base/plane.h
             ../base/transform.h
             ../base/bounding_box.h
             ../base/light.h
             ../base/sampler.h
             ../base/texture.h
             ../base/texture2d.h
             ../base/texture_cubemap.h
             ../base/fullscreen_quad.h)

set(BASE_SRC ../base/application.cpp 
             ../base/glsl_program.cpp 
             ../base/camera.cpp 
             ../base/transform.cpp
             ../base/texture.cpp
             ../base/texture2d.cpp
             ../base/texture_cubemap.cpp
             ../base/fullscreen_quad.cpp)

add_executable(pbr_viewer ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR} ${PROJECT_SHADERS})

source_group("Shader Files" FILES ${PROJECT_SHADERS})

add_custom_command(
    TARGET pbr_viewer
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    make_directory ${SHADER_TARGET_PATH}
    COMMENT "make directory ${SHADER_TARGET_PATH}"
)

set(first True)
foreach(shader ${PROJECT_SHADERS})
    get_filename_component(name ${shader} NAME)

    if (first)
        set(cmd_comment "create_hardlink shaders to ${SHADER_TARGET_PATH}\n")
        set(first False)
    else()
        set(cmd_comment "")
    endif()
    
    add_custom_command(
        TARGET pbr_viewer 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E 
        create_hardlink ${shader} ${SHADER_TARGET_PATH}/${name}
        COMMENT ${cmd_comment}
    )
endforeach(shader)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
elseif(XCODE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
else()
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Debug")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release")
    endif()
endif()

target_include_directories(pbr_viewer PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glm)
target_include_directories(pbr_viewer PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glad/include)
target_include_directories(pbr_viewer PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glfw/include)
target_include_directories(pbr_viewer PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/tinygltf)
target_include_directories(pbr_viewer PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/imgui)
target_include_directories(pbr_viewer PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/stb)

target_link_libraries(pbr_viewer glm)
target_link_libraries(pbr_viewer glad)
target_link_libraries(pbr_viewer glfw)
target_link_libraries(pbr_viewer tinygltf)
target_link_libraries(pbr_viewer imgui)
target_link_libraries(pbr_viewer stb)