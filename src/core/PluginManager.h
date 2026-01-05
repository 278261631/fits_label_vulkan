#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "IPlugin.h"  // 包含插件接口头文件，确保DestroyPluginFunc类型被识别

// 平台特定的动态库加载头文件
#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE LibraryHandle;
#else
    #include <dlfcn.h>
    typedef void* LibraryHandle;
#endif

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
    
    // 动态加载插件相关方法
    bool loadPluginsFromDirectory(const std::string& directory);
    bool loadPlugin(const std::string& pluginPath);
    
private:
    // 内部方法，实际执行插件目录加载
    bool loadPluginsFromDirectoryInternal(const std::string& directory);
    
    PluginContext* m_context;
    std::vector<IPlugin*> m_plugins;
    
    // 用于跟踪动态加载的插件库和对应的销毁函数
    struct DynamicPluginInfo {
        LibraryHandle libraryHandle;
        DestroyPluginFunc destroyFunc;
    };
    
    std::unordered_map<IPlugin*, DynamicPluginInfo> m_dynamicPlugins;
};
