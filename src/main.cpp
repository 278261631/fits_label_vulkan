#include "core/Application.h"
#include "core/Logger.h"
#include <iostream>

int main() {
    try {
        // 初始化日志系统
        Logger::init();
        
        Application app("GLFW + ImGui + Vulkan Demo", 800, 600);
        app.run();
        return 0;
    } catch (const std::exception& e) {
        Logger::error("Error: {}", e.what());
        return EXIT_FAILURE;
    }
}