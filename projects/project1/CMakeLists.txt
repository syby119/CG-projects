cmake_minimum_required(VERSION 3.10)

project(project1)

set(THIRD_PARTY_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/external)

file(GLOB PROJECT_HDR ./*.h)
file(GLOB PROJECT_SRC ./*.cpp)

set(BASE_HDR ../base/application.h
             ../base/frame_rate_indicator.h
             ../base/input.h
             ../base/glsl_program.h)

set(BASE_SRC ../base/application.cpp ../base/glsl_program.cpp)

add_executable(project1 ${PROJECT_SRC} ${PROJECT_HDR} ${BASE_SRC} ${BASE_HDR})

configure_project(${PROJECT_NAME})

target_include_directories(project1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glm)
target_include_directories(project1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glad/include)
target_include_directories(project1 PUBLIC ${THIRD_PARTY_LIBRARY_PATH}/glfw/include)

target_link_libraries(project1 glm)
target_link_libraries(project1 glad)
target_link_libraries(project1 glfw)