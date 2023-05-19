cmake_minimum_required(VERSION 3.24)
project(GraphicsPraktikum)

set(CMAKE_CXX_STANDARD 17)

add_executable(GraphicsPraktikum src/main.cpp src/vulkan/VulkanSetup.cpp src/vulkan/VulkanSetup.h src/vulkan/VulkanSettings.h src/vulkan/VulkanUtils.cpp src/vulkan/VulkanUtils.h src/utils/FileUtils.cpp src/utils/FileUtils.h src/vulkan/ApplicationContext.h src/vulkan/VulkanRenderer.cpp src/vulkan/VulkanRenderer.h src/vulkan/BufferImage.cpp src/vulkan/BufferImage.h src/scene/Scene.cpp src/scene/Scene.h src/scene/SceneSetup.cpp src/scene/SceneSetup.h src/scene/RenderableObject.cpp src/scene/RenderableObject.h src/scene/Camera.cpp src/scene/Camera.h src/rendering/RenderContext.h src/rendering/RenderSetup.cpp src/rendering/RenderSetup.h src/rendering/Shader.cpp src/rendering/Shader.h src/rendering/RenderSetupDescription.h src/scene/Model.h src/scene/Model.cpp src/rendering/host_device.h)

# add additional source code
target_sources(GraphicsPraktikum PUBLIC src/window.h src/window.cpp)

set(PROJECT_ROOT_DIR ${PROJECT_SOURCE_DIR})
message(${PROJECT_ROOT_DIR})
# Constants for lib includes
set(GLFW_DIR libs/glfw/)
set(IMGUI_DIR libs/imgui/)
set(GLM_DIR libs/glm/)
set(TINY_GLTF_DIR libs/tiny_gltf/)

# VulkanSDK
find_package(Vulkan REQUIRED)

# ImGUI
target_include_directories(GraphicsPraktikum PUBLIC
${PROJECT_ROOT_DIR}/src # include so local includes are relative to sources root
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

add_subdirectory(${TINY_GLTF_DIR})
target_link_libraries(GraphicsPraktikum tinygltf)

# Pass path to the Vulkan glslangValidator.exe as definition to C++
add_definitions(-DVULKAN_GLSLANG_VALIDATOR_PATH=\"${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}\")