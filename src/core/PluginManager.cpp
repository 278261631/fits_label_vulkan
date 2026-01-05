#include "PluginManager.h"
#include "IPlugin.h"
#include "PluginContext.h"
#include <iostream>
#include <algorithm>

PluginManager::PluginManager() : m_context(nullptr) {
}

PluginManager::~PluginManager() {
    cleanup();
}

bool PluginManager::init(PluginContext* context) {
    m_context = context;
    
    // 初始化所有已添加的插件
    for (auto plugin : m_plugins) {
        if (!plugin->init(m_context)) {
            std::cerr << "Failed to initialize plugin: " << plugin->getName() << std::endl;
            return false;
        }
    }
    
    return true;
}

void PluginManager::update(float deltaTime) {
    // 更新所有插件
    for (auto plugin : m_plugins) {
        plugin->update(deltaTime);
    }
}

void PluginManager::cleanup() {
    // 清理所有插件
    for (auto plugin : m_plugins) {
        plugin->cleanup();
    }
    
    m_plugins.clear();
    m_context = nullptr;
}

bool PluginManager::addPlugin(IPlugin* plugin) {
    if (!plugin) {
        std::cerr << "Invalid plugin pointer!" << std::endl;
        return false;
    }
    
    // 检查插件是否已存在
    if (findPlugin(plugin->getName()) != nullptr) {
        std::cerr << "Plugin already exists: " << plugin->getName() << std::endl;
        return false;
    }
    
    // 如果已经初始化，立即初始化插件
    if (m_context) {
        if (!plugin->init(m_context)) {
            std::cerr << "Failed to initialize plugin: " << plugin->getName() << std::endl;
            return false;
        }
    }
    
    m_plugins.push_back(plugin);
    std::cout << "Plugin added: " << plugin->getName() << " (v" << plugin->getVersion() << ")" << std::endl;
    
    return true;
}

bool PluginManager::removePlugin(IPlugin* plugin) {
    if (!plugin) {
        return false;
    }
    
    auto it = std::find(m_plugins.begin(), m_plugins.end(), plugin);
    if (it != m_plugins.end()) {
        plugin->cleanup();
        m_plugins.erase(it);
        std::cout << "Plugin removed: " << plugin->getName() << std::endl;
        return true;
    }
    
    return false;
}

bool PluginManager::removePlugin(const std::string& pluginName) {
    IPlugin* plugin = findPlugin(pluginName);
    return removePlugin(plugin);
}

IPlugin* PluginManager::findPlugin(const std::string& pluginName) const {
    for (auto plugin : m_plugins) {
        if (plugin->getName() == pluginName) {
            return plugin;
        }
    }
    return nullptr;
}
