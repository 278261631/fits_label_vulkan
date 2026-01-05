#pragma once

#include "IPlugin.h"
#include "PluginContext.h"
#include <string>

class DemoPlugin : public IPlugin {
public:
    DemoPlugin();
    ~DemoPlugin() override;
    
    bool init(PluginContext* context) override;
    void update(float deltaTime) override;
    void cleanup() override;
    
    const char* getName() const override;
    const char* getVersion() const override;
    
private:
    PluginContext* m_context;
    float m_fps;
    float m_frameTime;
    int m_frameCount;
    float m_timeSinceLastUpdate;
};
