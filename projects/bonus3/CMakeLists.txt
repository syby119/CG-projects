cmake_minimum_required(VERSION 3.20)

project(bonus3)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)
set(SHADER_TARGET_PATH ${CMAKE_BINARY_DIR}/media/shader/bonus3)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glm)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glad/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/glfw/include)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/tinyobjloader)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/imgui)

include_directories(${THIRD_PARTY_LIBRARY_PATH}/stb)

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)
file(GLOB PROJECT_SHADERS ./*.vert ./*.geom ./*.frag)

set(BASE_HDR ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h
             ../base/framebuffer.h
             ../base/transform.h
             ../base/camera.h
             ../base/bounding_box.h
             ../base/texture.h
             ../base/texture2d.h
             ../base/texture_cubemap.h
             ../base/model.h
             ../base/fullscreen_quad.h)

set(BASE_SRC ../base/application.cpp 
             ../base/glsl_program.cpp 
             ../base/transform.cpp
             ../base/camera.cpp
             ../base/texture.cpp
             ../base/texture2d.cpp
             ../base/texture_cubemap.cpp
             ../base/fullscreen_quad.cpp
             ../base/model.cpp)

add_executable(bonus3 ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR} ${PROJECT_SHADERS})

source_group("Shader Files" FILES ${PROJECT_SHADERS})

add_custom_command(
    TARGET bonus3
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    make_directory ${SHADER_TARGET_PATH}
    COMMENT "make directory ${SHADER_TARGET_PATH}"
)

set(first True)
foreach(shader ${PROJECT_SHADERS})
    get_filename_component(name ${shader} NAME)
    
    if (first)
        set(cmd_comment "create_hardlink shaders to ${CMAKE_BINARY_DIR}/media/shader/bonus3\n")
        set(first False)
    else()
        set(cmd_comment "")
    endif()

    add_custom_command(
        TARGET bonus3 
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E 
        create_hardlink ${shader} ${CMAKE_BINARY_DIR}/media/shader/bonus3/${name}
        COMMENT ${cmd_comment}
    )
endforeach(shader)

if(WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
elseif(UNIX)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Debug")
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Release")
endif()
endif()

target_link_libraries(bonus3 glm)

target_link_libraries(bonus3 glad)

target_link_libraries(bonus3 glfw)

target_link_libraries(bonus3 tinyobjloader)

target_link_libraries(bonus3 imgui)

target_link_libraries(bonus3 stb)