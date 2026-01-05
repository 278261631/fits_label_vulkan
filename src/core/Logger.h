#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <string>

class Logger {
public:
    enum class Level {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

    static void init();
    static void setLevel(Level level);
    
    template<typename... Args>
    static void trace(const char* fmt, Args&&... args) {
        spdlog::trace(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void debug(const char* fmt, Args&&... args) {
        spdlog::debug(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(const char* fmt, Args&&... args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(const char* fmt, Args&&... args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const char* fmt, Args&&... args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void critical(const char* fmt, Args&&... args) {
        spdlog::critical(fmt, std::forward<Args>(args)...);
    }
    
    template<typename T>
    static void trace(const T& msg) {
        spdlog::trace(msg);
    }
    
    template<typename T>
    static void debug(const T& msg) {
        spdlog::debug(msg);
    }
    
    template<typename T>
    static void info(const T& msg) {
        spdlog::info(msg);
    }
    
    template<typename T>
    static void warn(const T& msg) {
        spdlog::warn(msg);
    }
    
    template<typename T>
    static void error(const T& msg) {
        spdlog::error(msg);
    }
    
    template<typename T>
    static void critical(const T& msg) {
        spdlog::critical(msg);
    }

private:
    Logger() = delete;
    ~Logger() = delete;
};
