#include "Application.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "UI.h"
#include "Camera.h"
#include "Config.h"
#include "PluginManager.h"
#include "PluginContext.h"
#include "DemoPlugin.h"
#include <iostream>
#include <thread>

Application::Application(const std::string& title, int width, int height)
    : m_title(title), m_width(width), m_height(height), m_running(false),
      m_vulkanContext(nullptr), m_renderer(nullptr), m_inputHandler(nullptr),
      m_ui(nullptr), m_camera(nullptr),
      m_pluginContext(nullptr), m_pluginManager(std::make_unique<PluginManager>()) {}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    try {
        // 配置系统已经在run()方法中初始化
        Config& config = Config::getInstance();
        
        // 初始化Vulkan上下文
        m_vulkanContext = std::make_unique<VulkanContext>(m_width, m_height, m_title.c_str());
        if (!m_vulkanContext->init()) {
            std::cerr << "Failed to initialize Vulkan context!" << std::endl;
            return false;
        }

        // 创建相机
        m_camera = std::make_unique<Camera>(m_width, m_height);

        // 初始化输入处理
        m_inputHandler = std::make_unique<InputHandler>(m_vulkanContext->getWindow(), m_camera.get());
        m_inputHandler->init();

        // 初始化渲染器，先不传递UI指针
        m_renderer = std::make_unique<Renderer>(m_vulkanContext.get(), m_camera.get(), nullptr);
        if (!m_renderer->init()) {
            std::cerr << "Failed to initialize renderer!" << std::endl;
            return false;
        }
        
        // 创建并初始化UI
        m_ui = std::make_unique<UI>(m_vulkanContext.get(), m_renderer.get(), m_camera.get());
        if (!m_ui->init()) {
            std::cerr << "Failed to initialize UI!" << std::endl;
            return false;
        }
        
        // 设置渲染器的UI指针
        m_renderer->setUI(m_ui.get());
        
        // 初始化插件系统
        m_pluginContext = std::make_unique<PluginContext>(
            m_vulkanContext.get(),
            m_renderer.get(),
            m_camera.get()
        );
        
        // 初始化插件管理器
        if (!m_pluginManager->init(m_pluginContext.get())) {
            std::cerr << "Failed to initialize plugin manager!" << std::endl;
            return false;
        }
        
        // 添加Demo插件
        auto demoPlugin = new DemoPlugin();
        if (!m_pluginManager->addPlugin(demoPlugin)) {
            std::cerr << "Failed to add DemoPlugin!" << std::endl;
            delete demoPlugin;
            return false;
        }
        
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
    Config& config = Config::getInstance();
    
    // 先加载配置，然后再根据配置决定是否输出日志
    config.load();
    
    if (config.isDebugMode()) {
        std::cout << "Entering Application::run..." << std::endl;
    }
    
    if (!init()) {
        if (config.isDebugMode()) {
            std::cout << "Application::init failed!" << std::endl;
        }
        shutdown();
        return;
    }
    
    if (config.isDebugMode()) {
        std::cout << "Application::init succeeded, entering mainLoop..." << std::endl;
    }
    
    mainLoop();
    
    if (config.isDebugMode()) {
        std::cout << "mainLoop exited, shutting down..." << std::endl;
    }
    shutdown();
    
    if (config.isDebugMode()) {
        std::cout << "Application::run completed!" << std::endl;
    }
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
        
        // 计算deltaTime（秒）
        float deltaTime = frameDuration.count() / 1000.0f;
        
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
        
        // 更新插件
        m_pluginManager->update(deltaTime);
        
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
    // 智能指针会自动释放内存，无需手动delete
    m_running = false;
}
