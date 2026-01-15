#include "Application.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "InputHandler.h"
#include "UI.h"
#include "Camera.h"
#include "Config.h"
#include "Logger.h"
#include "PluginManager.h"
#include "PluginContext.h"
#include "DemoPlugin.h"
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
            Logger::error("Failed to initialize Vulkan context!");
            return false;
        }

        // 创建相机
        m_camera = std::make_unique<Camera>(m_width, m_height);

        // 初始化输入处理（先用nullptr，稍后设置UI）
        m_inputHandler = std::make_unique<InputHandler>(m_vulkanContext->getWindow(), m_camera.get());
        m_inputHandler->init();

        // 初始化渲染器，先不传递UI指针
        m_renderer = std::make_unique<Renderer>(m_vulkanContext.get(), m_camera.get(), nullptr);
        if (!m_renderer->init()) {
            Logger::error("Failed to initialize renderer!");
            return false;
        }
        
        // 创建并初始化UI
        m_ui = std::make_unique<UI>(m_vulkanContext.get(), m_renderer.get(), m_camera.get());
        if (!m_ui->init()) {
            Logger::error("Failed to initialize UI!");
            return false;
        }

        // 设置渲染器的UI指针
        m_renderer->setUI(m_ui.get());

        // 设置UI的GridRenderer指针
        m_ui->setGridRenderer(m_renderer->getGridRenderer());
        
        // 设置输入处理器的UI指针
        m_inputHandler->setUI(m_ui.get());
        
        // 初始化插件系统
        m_pluginContext = std::make_unique<PluginContext>(
            m_vulkanContext.get(),
            m_renderer.get(),
            m_camera.get()
        );

        // 设置渲染器的PluginContext
        m_renderer->setPluginContext(m_pluginContext.get());

        // 初始化插件管理器
        if (!m_pluginManager->init(m_pluginContext.get())) {
            Logger::error("Failed to initialize plugin manager!");
            return false;
        }
        
        // 加载外部插件
        m_pluginManager->loadPluginsFromDirectory("./plugins");
        
        // 添加Demo插件
        auto demoPlugin = new DemoPlugin();
        if (!m_pluginManager->addPlugin(demoPlugin)) {
            Logger::error("Failed to add DemoPlugin!");
            delete demoPlugin;
            return false;
        }
        
        if (config.isDebugMode()) {
            Logger::info("=== Camera Controls ===");
            Logger::info("- Right Mouse + Drag: Rotate camera");
            Logger::info("- Middle Mouse + Drag: Pan camera");
            Logger::info("- Mouse Wheel: Zoom");
            Logger::info("- ESC: Exit");
            Logger::info("=======================");
        }

        m_running = true;
        return true;
    } catch (const std::exception& e) {
        Logger::error("Initialization error: {}", e.what());
        return false;
    }
}

void Application::run() {
    Config& config = Config::getInstance();
    
    // 先加载配置，然后再根据配置决定是否输出日志
    config.load();
    
    Logger::debug("Entering Application::run...");
    
    if (!init()) {
        Logger::debug("Application::init failed!");
        shutdown();
        return;
    }
    
    Logger::debug("Application::init succeeded, entering mainLoop...");
    
    mainLoop();
    
    Logger::debug("mainLoop exited, shutting down...");
    shutdown();
    
    Logger::debug("Application::run completed!");
}

void Application::mainLoop() {
    Config& config = Config::getInstance();
    
    Logger::debug("Entering mainLoop...");
    
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
        
        Logger::trace("Frame {} - polling events...", frameCount++);
        
        m_inputHandler->pollEvents();
        
        Logger::trace("Frame {} - updating camera...", frameCount);
        
        m_camera->update();
        
        // 更新插件
        m_pluginManager->update(deltaTime);
        
        Logger::trace("Frame {} - rendering and updating UI...", frameCount);
        
        m_renderer->render();
        
        Logger::trace("Frame {} - completed!", frameCount);
        
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
    
    Logger::debug("Exiting mainLoop...");
}

void Application::shutdown() {
    // 智能指针会自动释放内存，无需手动delete
    m_running = false;
}
