#pragma once

class PluginContext;

class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    virtual bool init(PluginContext* context) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void cleanup() = 0;
    
    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;
};

// 插件创建函数的typedef
typedef IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(IPlugin*);
