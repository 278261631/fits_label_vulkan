#include "Application.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "UI.h"
#include "Camera.h"
#include <iostream>

Application::Application(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_running(false),
      m_vulkanContext(nullptr), m_renderer(nullptr), m_inputHandler(nullptr),
      m_ui(nullptr), m_camera(nullptr) {}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    try {
        // 初始化Vulkan上下文
        m_vulkanContext = new VulkanContext(m_width, m_height, m_title.c_str());
        if (!m_vulkanContext->init()) {
            std::cerr << "Failed to initialize Vulkan context!" << std::endl;
            return false;
        }

        // 创建相机
        m_camera = new Camera(m_width, m_height);

        // 初始化输入处理
        m_inputHandler = new InputHandler(m_vulkanContext->getWindow(), m_camera);
        m_inputHandler->init();

        // 初始化渲染器（暂时不使用UI）
        m_renderer = new Renderer(m_vulkanContext, m_camera, nullptr);
        if (!m_renderer->init()) {
            std::cerr << "Failed to initialize renderer!" << std::endl;
            return false;
        }

        // 暂时不初始化UI，先确保核心渲染正常工作
        m_ui = nullptr;

        m_running = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

void Application::run() {
    std::cout << "Entering Application::run..." << std::endl;
    
    if (!init()) {
        std::cout << "Application::init failed!" << std::endl;
        shutdown();
        return;
    }
    
    std::cout << "Application::init succeeded, entering mainLoop..." << std::endl;
    
    mainLoop();
    
    std::cout << "mainLoop exited, shutting down..." << std::endl;
    shutdown();
    
    std::cout << "Application::run completed!" << std::endl;
}

void Application::mainLoop() {
    std::cout << "Entering mainLoop..." << std::endl;
    
    int frameCount = 0;
    
    while (m_running && !m_vulkanContext->shouldClose()) {
        std::cout << "Frame " << frameCount++ << " - polling events..." << std::endl;
        m_inputHandler->pollEvents();
        
        std::cout << "Frame " << frameCount << " - updating camera..." << std::endl;
        m_camera->update();
        
        std::cout << "Frame " << frameCount << " - rendering and updating UI..." << std::endl;
        m_renderer->render();
        
        std::cout << "Frame " << frameCount << " - completed!" << std::endl;
    }
    
    std::cout << "Exiting mainLoop..." << std::endl;
}

void Application::shutdown() {
    if (m_ui) {
        delete m_ui;
        m_ui = nullptr;
    }

    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }

    if (m_inputHandler) {
        delete m_inputHandler;
        m_inputHandler = nullptr;
    }

    if (m_camera) {
        delete m_camera;
        m_camera = nullptr;
    }

    if (m_vulkanContext) {
        delete m_vulkanContext;
        m_vulkanContext = nullptr;
    }

    m_running = false;
}
