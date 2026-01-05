#include "PluginManager.h"
#include "IPlugin.h"
#include "PluginContext.h"
#include "Logger.h"
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
            Logger::error("Failed to initialize plugin: {}", plugin->getName());
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
        Logger::error("Invalid plugin pointer!");
        return false;
    }
    
    // 检查插件是否已存在
    if (findPlugin(plugin->getName()) != nullptr) {
        Logger::error("Plugin already exists: {}", plugin->getName());
        return false;
    }
    
    // 如果已经初始化，立即初始化插件
    if (m_context) {
        if (!plugin->init(m_context)) {
            Logger::error("Failed to initialize plugin: {}", plugin->getName());
            return false;
        }
    }
    
    m_plugins.push_back(plugin);
    Logger::info("Plugin added: {} (v{})", plugin->getName(), plugin->getVersion());
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
        Logger::info("Plugin removed: {}", plugin->getName());
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
