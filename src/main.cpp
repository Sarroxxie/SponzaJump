#include <iostream>
#include "window.h"

#define DEFAULT_APPLICATION_WIDTH 800
#define DEFAULT_APPLICATION_HEIGHT 600
#define DEFAULT_APPLICATION_NAME "GraphicsPraktikum"

int main() {
    // macro is defined by cmake
    std::cout << "glslangValidator path: " << VULKAN_GLSLANG_VALIDATOR_PATH << "\n";

    Window window = Window(DEFAULT_APPLICATION_WIDTH,
                           DEFAULT_APPLICATION_HEIGHT, DEFAULT_APPLICATION_NAME);
    window.render();

    return 0;
}
