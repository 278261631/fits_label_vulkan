#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

Config::Config() {
    // 设置默认值
    setFPS(DEFAULT_FPS);
    setDebugMode(DEFAULT_DEBUG_MODE);
}

Config::~Config() {
    // 保存配置
    save();
}

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        // 解析键值对
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // 去除空格
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        m_config[key] = value;
    }

    file.close();
    if (isDebugMode()) {
        std::cout << "Config loaded from " << filename << std::endl;
    }
    return true;
}

bool Config::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file for writing: " << filename << std::endl;
        return false;
    }

    // 写入配置
    for (const auto& pair : m_config) {
        file << pair.first << " = " << pair.second << std::endl;
    }

    file.close();
    if (isDebugMode()) {
        std::cout << "Config saved to " << filename << std::endl;
    }
    return true;
}

void Config::setInt(const std::string& key, int value) {
    m_config[key] = std::to_string(value);
}

void Config::setBool(const std::string& key, bool value) {
    m_config[key] = value ? "true" : "false";
}

void Config::setFloat(const std::string& key, float value) {
    m_config[key] = std::to_string(value);
}

void Config::setString(const std::string& key, const std::string& value) {
    m_config[key] = value;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        const std::string& value = it->second;
        if (value == "true" || value == "1" || value == "yes") {
            return true;
        } else if (value == "false" || value == "0" || value == "no") {
            return false;
        }
    }
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        try {
            return std::stof(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_config.find(key);
    if (it != m_config.end()) {
        return it->second;
    }
    return defaultValue;
}

void Config::setFPS(int fps) {
    setInt("fps", fps);
}

int Config::getFPS() const {
    return getInt("fps", DEFAULT_FPS);
}

void Config::setDebugMode(bool debug) {
    setBool("debug_mode", debug);
}

bool Config::isDebugMode() const {
    return getBool("debug_mode", DEFAULT_DEBUG_MODE);
}
