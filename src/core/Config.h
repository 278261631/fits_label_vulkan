#pragma once

#include <string>
#include <map>

class Config {
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    // 加载配置
    bool load(const std::string& filename = "config.ini");
    
    // 保存配置
    bool save(const std::string& filename = "config.ini");

    // 设置参数
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);
    void setFloat(const std::string& key, float value);
    void setString(const std::string& key, const std::string& value);

    // 获取参数
    int getInt(const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;

    // 动态参数
    void setFPS(int fps);
    int getFPS() const;
    
    void setDebugMode(bool debug);
    bool isDebugMode() const;
    
    void setLogLevel(int level);
    int getLogLevel() const;

private:
    Config();
    ~Config();

    // 禁止复制和赋值
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::map<std::string, std::string> m_config;
    
    // 动态参数默认值
    static constexpr int DEFAULT_FPS = 60;
    static constexpr bool DEFAULT_DEBUG_MODE = true;
    static constexpr int DEFAULT_LOG_LEVEL = 2; // 0: Trace, 1: Debug, 2: Info, 3: Warn, 4: Error, 5: Critical
};
