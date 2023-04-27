cmake_minimum_required(VERSION 3.24)
project(GraphicsPraktikum)

set(CMAKE_CXX_STANDARD 17)

add_executable(GraphicsPraktikum src/main.cpp)

# add additional source code
target_sources(GraphicsPraktikum PUBLIC src/window.h src/window.cpp)

# Constants for lib includes
set(GLFW_DIR libs/glfw/)
set(IMGUI_DIR libs/imgui/)
set(GLM_DIR libs/glm/)

# VulkanSDK
find_package(Vulkan REQUIRED)

# ImGUI
target_include_directories(GraphicsPraktikum PUBLIC 
${IMGUI_DIR} 
${IMGUI_DIR}/backends 
)

target_sources(GraphicsPraktikum PUBLIC 
"${IMGUI_DIR}/imgui.h"
"${IMGUI_DIR}/imgui.cpp"
"${IMGUI_DIR}/imgui_draw.cpp"
"${IMGUI_DIR}/imgui_tables.cpp"
"${IMGUI_DIR}/imgui_widgets.cpp"
"${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
"${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
)

# We only use glfw, no need to test it or build the docs for us
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add and link libs
add_subdirectory(${GLFW_DIR})
target_link_libraries(GraphicsPraktikum glfw ${GLFW_LIBRARIES})

target_link_libraries(GraphicsPraktikum Vulkan::Vulkan)

add_subdirectory(${GLM_DIR})
target_link_libraries(GraphicsPraktikum glm)

# Pass path to the Vulkan glslangValidator.exe as definition to C++
add_definitions(-DVULKAN_GLSLANG_VALIDATOR_PATH=\"${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}\")