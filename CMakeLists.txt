cmake_minimum_required(VERSION 2.6)

project(demo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

find_package(OpenGL REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLM REQUIRED)

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLFW3_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR})

add_subdirectory(common)
add_subdirectory(spiral)
add_subdirectory(cube)
add_subdirectory(xcube)
add_subdirectory(rubik)
add_subdirectory(slices)
add_subdirectory(strips)
add_subdirectory(shadowmap)
add_subdirectory(slices-shadows)
