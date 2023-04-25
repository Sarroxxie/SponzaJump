cmake_minimum_required(VERSION 3.24)
project(GraphicsPraktikum)

set(CMAKE_CXX_STANDARD 17)

add_executable(GraphicsPraktikum src/main.cpp)

# Constants for lib includes
set(GLFW_DIR libs/glfw/)
set(IMGUI_DIR libs/imgui/)

# VulkanSDK
find_package(Vulkan REQUIRED)

# ImGUI
add_library(imgui STATIC)
target_sources(imgui PUBLIC 
"${IMGUI_DIR}/imgui.h"
"${IMGUI_DIR}/imgui.cpp"

"${IMGUI_DIR}/imgui_draw.cpp"
"${IMGUI_DIR}/imgui_tables.cpp"
"${IMGUI_DIR}/imgui_widgets.cpp"

"${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
"${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
)

target_include_directories(imgui PUBLIC 
${IMGUI_DIR} 
${IMGUI_DIR}/backends 
${GLFW_DIR}/include 
${Vulkan_INCLUDE_DIRS}
)

# We only use glfw, no need to test it or build the docs for us
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add and link libs
add_subdirectory(${GLFW_DIR})
target_link_libraries(GraphicsPraktikum glfw ${GLFW_LIBRARIES})

target_link_libraries(GraphicsPraktikum imgui)
target_link_libraries(GraphicsPraktikum Vulkan::Vulkan)