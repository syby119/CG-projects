cmake_minimum_required(VERSION 3.20)

project(bonus1)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)
set(SHADER_TARGET_PATH ${CMAKE_BINARY_DIR}/media/shader/bonus1)

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
             ../base/model.h
             ../base/bounding_box.h
             ../base/fullscreen_quad.h
             ../base/vertex.h
             ../base/light.h
             ../base/texture.h
             ../base/texture2d.h)

set(BASE_SRC ../base/application.cpp
             ../base/glsl_program.cpp
             ../base/camera.cpp
             ../base/transform.cpp
             ../base/fullscreen_quad.cpp
             ../base/model.cpp
             ../base/texture.cpp
             ../base/texture2d.cpp)

add_executable(bonus1 ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR} ${PROJECT_SHADERS})

source_group("Shader Files" FILES ${PROJECT_SHADERS})

add_custom_command(
    TARGET bonus1
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
        TARGET bonus1 
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

target_include_directories(bonus1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glm)
target_include_directories(bonus1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glad/include)
target_include_directories(bonus1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glfw/include)
target_include_directories(bonus1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/tinyobjloader)
target_include_directories(bonus1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/imgui)
target_include_directories(bonus1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/stb)

target_link_libraries(bonus1 glm)
target_link_libraries(bonus1 glad)
target_link_libraries(bonus1 glfw)
target_link_libraries(bonus1 tinyobjloader)
target_link_libraries(bonus1 imgui)
target_link_libraries(bonus1 stb)