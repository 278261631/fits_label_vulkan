#include "core/Application.h"
#include <iostream>

int main() {
    try {
        Application app("GLFW + ImGui + Vulkan Demo", 800, 600);
        app.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}