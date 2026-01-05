#include "DemoPlugin.h"
#include <iostream>
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
    
    std::cout << "DemoPlugin initialized!" << std::endl;
    std::cout << "This plugin demonstrates how to use the plugin system." << std::endl;
    std::cout << "It will display FPS information in the console." << std::endl;
    
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
        std::cout << "[DemoPlugin] FPS: " << m_fps << ", Frame Time: " << (m_frameTime * 1000.0f) << "ms" << std::endl;
        
        // 重置计数器
        m_frameCount = 0;
        m_timeSinceLastUpdate = 0.0f;
    }
}

void DemoPlugin::cleanup() {
    std::cout << "DemoPlugin cleaned up!" << std::endl;
    m_context = nullptr;
}

const char* DemoPlugin::getName() const {
    return "DemoPlugin";
}

const char* DemoPlugin::getVersion() const {
    return "1.0.0";
}
