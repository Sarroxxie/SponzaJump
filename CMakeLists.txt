cmake_minimum_required(VERSION 3.24)
project(GraphicsPraktikum)

set(CMAKE_CXX_STANDARD 17)

add_executable(GraphicsPraktikum src/main.cpp)

# Constants for lib includes
set(GLFW_DIR libs/glfw/)

# We only use glfw, no need to test it or build the docs for us
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add and link libs
add_subdirectory(${GLFW_DIR})
target_link_libraries(GraphicsPraktikum glfw ${GLFW_LIBRARIES})


