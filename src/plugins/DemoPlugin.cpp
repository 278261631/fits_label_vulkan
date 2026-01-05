#include "DemoPlugin.h"
#include "Logger.h"
#include <chrono>

DemoPlugin::DemoPlugin() : 
    m_context(nullptr),
    m_fps(0.0f),
    m_frameTime(0.0f),
    m_frameCount(0),
    m_timeSinceLastUpdate(0.0f) {
}

DemoPlugin::~DemoPlugin() {
}

bool DemoPlugin::init(PluginContext* context) {
    m_context = context;
    
    Logger::info("DemoPlugin initialized!");
    Logger::info("This plugin demonstrates how to use the plugin system.");
    Logger::info("It will display FPS information in the console.");
    
    return true;
}

void DemoPlugin::update(float deltaTime) {
    // 更新帧计数和时间
    m_frameCount++;
    m_timeSinceLastUpdate += deltaTime;
    m_frameTime = deltaTime;
    
    // 每秒更新一次FPS
    if (m_timeSinceLastUpdate >= 1.0f) {
        m_fps = static_cast<float>(m_frameCount) / m_timeSinceLastUpdate;
        
        // 在控制台显示FPS信息
        Logger::info("[DemoPlugin] FPS: {}, Frame Time: {}ms", m_fps, (m_frameTime * 1000.0f));
        
        // 重置计数器
        m_frameCount = 0;
        m_timeSinceLastUpdate = 0.0f;
    }
}

void DemoPlugin::cleanup() {
    Logger::info("DemoPlugin cleaned up!");
    m_context = nullptr;
}

const char* DemoPlugin::getName() const {
    return "DemoPlugin";
}

const char* DemoPlugin::getVersion() const {
    return "1.0.0";
}
