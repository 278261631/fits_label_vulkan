#pragma once

#include <vector>
#include <string>
#include <memory>

class IPlugin;
class PluginContext;

class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    bool init(PluginContext* context);
    void update(float deltaTime);
    void cleanup();
    
    // 添加插件（静态链接方式）
    bool addPlugin(IPlugin* plugin);
    
    // 移除插件
    bool removePlugin(IPlugin* plugin);
    bool removePlugin(const std::string& pluginName);
    
    // 获取所有插件
    const std::vector<IPlugin*>& getPlugins() const { return m_plugins; }
    
    // 根据名称查找插件
    IPlugin* findPlugin(const std::string& pluginName) const;
    
private:
    PluginContext* m_context;
    std::vector<IPlugin*> m_plugins;
};
