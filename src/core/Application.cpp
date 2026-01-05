#include "Application.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "UI.h"
#include "Camera.h"
#include "Config.h"
#include <iostream>
#include <thread>

Application::Application(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_running(false),
      m_vulkanContext(nullptr), m_renderer(nullptr), m_inputHandler(nullptr),
      m_ui(nullptr), m_camera(nullptr) {}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    try {
        // 初始化配置系统
        Config& config = Config::getInstance();
        config.load();
        
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

        // 初始化渲染器，先不传递UI指针
        m_renderer = new Renderer(m_vulkanContext, m_camera, nullptr);
        if (!m_renderer->init()) {
            std::cerr << "Failed to initialize renderer!" << std::endl;
            return false;
        }
        
        // 创建并初始化UI
        m_ui = new UI(m_vulkanContext, m_renderer, m_camera);
        if (!m_ui->init()) {
            std::cerr << "Failed to initialize UI!" << std::endl;
            return false;
        }
        
        // 设置渲染器的UI指针
        m_renderer->setUI(m_ui);
        
        if (config.isDebugMode()) {
            std::cout << "=== 相机控制说明 ===" << std::endl;
            std::cout << "- 左键 + 拖动: 旋转相机" << std::endl;
            std::cout << "- 中键 + 拖动: 平移相机" << std::endl;
            std::cout << "- 鼠标滚轮: 缩放" << std::endl;
            std::cout << "- ESC: 退出程序" << std::endl;
            std::cout << "==================" << std::endl;
        }

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
    Config& config = Config::getInstance();
    
    if (config.isDebugMode()) {
        std::cout << "Entering mainLoop..." << std::endl;
    }
    
    int frameCount = 0;
    
    // FPS控制相关变量
    using namespace std::chrono;
    steady_clock::time_point lastFrameTime = steady_clock::now();
    
    while (m_running && !m_vulkanContext->shouldClose()) {
        // 记录当前帧开始时间
        steady_clock::time_point currentFrameTime = steady_clock::now();
        
        // 计算上一帧耗时（毫秒）
        duration<float, std::milli> frameDuration = currentFrameTime - lastFrameTime;
        
        // 计算目标帧时间（毫秒）
        int targetFPS = config.getFPS();
        float targetFrameTime = 1000.0f / targetFPS;
        
        if (config.isDebugMode()) {
            std::cout << "Frame " << frameCount++ << " - polling events..." << std::endl;
        }
        
        m_inputHandler->pollEvents();
        
        if (config.isDebugMode()) {
            std::cout << "Frame " << frameCount << " - updating camera..." << std::endl;
        }
        
        m_camera->update();
        
        if (config.isDebugMode()) {
            std::cout << "Frame " << frameCount << " - rendering and updating UI..." << std::endl;
        }
        
        m_renderer->render();
        
        if (config.isDebugMode()) {
            std::cout << "Frame " << frameCount << " - completed!" << std::endl;
        }
        
        // 更新上一帧时间
        lastFrameTime = steady_clock::now();
        
        // 计算当前帧总耗时
        duration<float, std::milli> totalFrameDuration = lastFrameTime - currentFrameTime;
        
        // 如果帧耗时小于目标帧时间，休眠剩余时间
        if (totalFrameDuration.count() < targetFrameTime) {
            float sleepTime = targetFrameTime - totalFrameDuration.count();
            std::this_thread::sleep_for(milliseconds(static_cast<long long>(sleepTime)));
        }
    }
    
    if (config.isDebugMode()) {
        std::cout << "Exiting mainLoop..." << std::endl;
    }
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
