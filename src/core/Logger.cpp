#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

void Logger::init() {
    // 创建控制台和文件双sink
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("fits_label.log", true);
    
    // 设置日志格式
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    
    // 创建logger
    auto logger = std::make_shared<spdlog::logger>("fits_label", spdlog::sinks_init_list({console_sink, file_sink}));
    
    // 设置为默认logger
    spdlog::set_default_logger(logger);
    
    // 默认日志级别
    spdlog::set_level(spdlog::level::info);
    
    // 开启日志刷新
    spdlog::flush_on(spdlog::level::info);
}

void Logger::setLevel(Level level) {
    spdlog::level::level_enum spd_level;
    
    switch (level) {
        case Level::Trace: 
            spd_level = spdlog::level::trace;
            break;
        case Level::Debug: 
            spd_level = spdlog::level::debug;
            break;
        case Level::Info: 
            spd_level = spdlog::level::info;
            break;
        case Level::Warn: 
            spd_level = spdlog::level::warn;
            break;
        case Level::Error: 
            spd_level = spdlog::level::err;
            break;
        case Level::Critical: 
            spd_level = spdlog::level::critical;
            break;
        default: 
            spd_level = spdlog::level::info;
            break;
    }
    
    spdlog::set_level(spd_level);
}
