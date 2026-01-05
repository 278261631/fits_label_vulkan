#include "PluginManager.h"
#include "IPlugin.h"
#include "PluginContext.h"
#include "Logger.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

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
        
        // 如果是动态加载的插件，调用其销毁函数并关闭库
        auto it = m_dynamicPlugins.find(plugin);
        if (it != m_dynamicPlugins.end()) {
            if (it->second.destroyFunc) {
                it->second.destroyFunc(plugin);
            }
            
            #ifdef _WIN32
                FreeLibrary(it->second.libraryHandle);
            #else
                dlclose(it->second.libraryHandle);
            #endif
        }
    }
    
    m_plugins.clear();
    m_dynamicPlugins.clear();
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
        
        // 如果是动态加载的插件，调用其销毁函数并关闭库
        auto dynIt = m_dynamicPlugins.find(plugin);
        if (dynIt != m_dynamicPlugins.end()) {
            if (dynIt->second.destroyFunc) {
                dynIt->second.destroyFunc(plugin);
            }
            
            #ifdef _WIN32
                FreeLibrary(dynIt->second.libraryHandle);
            #else
                dlclose(dynIt->second.libraryHandle);
            #endif
            
            m_dynamicPlugins.erase(dynIt);
        }
        
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

bool PluginManager::loadPlugin(const std::string& pluginPath) {
    Logger::info("Loading plugin from: {}", pluginPath);
    
    // 打开动态库
    LibraryHandle handle;
    #ifdef _WIN32
        handle = LoadLibraryA(pluginPath.c_str());
    #else
        handle = dlopen(pluginPath.c_str(), RTLD_LAZY | RTLD_LOCAL);
    #endif
    
    if (!handle) {
        #ifdef _WIN32
            char errorMsg[256];
            FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                          errorMsg, sizeof(errorMsg), NULL);
            Logger::error("Failed to load plugin: {}. Error: {}", pluginPath, errorMsg);
        #else
            Logger::error("Failed to load plugin: {}. Error: {}", pluginPath, dlerror());
        #endif
        return false;
    }
    
    // 获取插件创建函数
    CreatePluginFunc createFunc;
    #ifdef _WIN32
        createFunc = (CreatePluginFunc)GetProcAddress(handle, "createPlugin");
    #else
        createFunc = (CreatePluginFunc)dlsym(handle, "createPlugin");
    #endif
    
    if (!createFunc) {
        Logger::error("Failed to find createPlugin function in plugin: {}", pluginPath);
        #ifdef _WIN32
            FreeLibrary(handle);
        #else
            dlclose(handle);
        #endif
        return false;
    }
    
    // 获取插件销毁函数
    DestroyPluginFunc destroyFunc;
    #ifdef _WIN32
        destroyFunc = (DestroyPluginFunc)GetProcAddress(handle, "destroyPlugin");
    #else
        destroyFunc = (DestroyPluginFunc)dlsym(handle, "destroyPlugin");
    #endif
    
    if (!destroyFunc) {
        Logger::error("Failed to find destroyPlugin function in plugin: {}", pluginPath);
        #ifdef _WIN32
            FreeLibrary(handle);
        #else
            dlclose(handle);
        #endif
        return false;
    }
    
    // 创建插件实例
    IPlugin* plugin = createFunc();
    if (!plugin) {
        Logger::error("Failed to create plugin instance from: {}", pluginPath);
        #ifdef _WIN32
            FreeLibrary(handle);
        #else
            dlclose(handle);
        #endif
        return false;
    }
    
    // 添加插件到管理器
    if (!addPlugin(plugin)) {
        Logger::error("Failed to add plugin from: {}", pluginPath);
        destroyFunc(plugin);
        #ifdef _WIN32
            FreeLibrary(handle);
        #else
            dlclose(handle);
        #endif
        return false;
    }
    
    // 存储动态插件信息
    DynamicPluginInfo pluginInfo;
    pluginInfo.libraryHandle = handle;
    pluginInfo.destroyFunc = destroyFunc;
    m_dynamicPlugins[plugin] = pluginInfo;
    
    Logger::info("Successfully loaded plugin: {}", pluginPath);
    return true;
}

bool PluginManager::loadPluginsFromDirectory(const std::string& directory) {
    Logger::info("Loading plugins from directory: {}", directory);
    
    // 检查目录是否存在
    if (fs::exists(directory) && fs::is_directory(directory)) {
        return loadPluginsFromDirectoryInternal(directory);
    }
    
    // 尝试多种方式获取可执行文件所在目录
    std::string exeDirPlugins;
    
    // 方式1: 使用GetModuleFileName获取可执行文件路径（Windows）
    #ifdef _WIN32
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeFullPath = exePath;
        std::string exeDir = fs::path(exeFullPath).parent_path().string();
        exeDirPlugins = exeDir + "/plugins";
    #else
        // 方式2: 使用/proc/self/exe（Linux）或_NSGetExecutablePath（macOS）
        std::string exePath;
        char buffer[PATH_MAX];
        #ifdef __linux__
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer)-1);
            if (len != -1) {
                buffer[len] = '\0';
                exePath = buffer;
            }
        #elif defined(__APPLE__)
            uint32_t size = sizeof(buffer);
            if (_NSGetExecutablePath(buffer, &size) == 0) {
                exePath = buffer;
            }
        #endif
        
        if (!exePath.empty()) {
            std::string exeDir = fs::path(exePath).parent_path().string();
            exeDirPlugins = exeDir + "/plugins";
        }
    #endif
    
    // 检查可执行文件目录下的plugins目录是否存在
    if (!exeDirPlugins.empty() && fs::exists(exeDirPlugins) && fs::is_directory(exeDirPlugins)) {
        Logger::info("Trying plugin directory in executable path: {}", exeDirPlugins);
        return loadPluginsFromDirectoryInternal(exeDirPlugins);
    }
    
    // 检查当前工作目录下的plugins目录
    std::string cwdPlugins = fs::current_path().string() + "/plugins";
    if (fs::exists(cwdPlugins) && fs::is_directory(cwdPlugins)) {
        Logger::info("Trying plugin directory in current working directory: {}", cwdPlugins);
        return loadPluginsFromDirectoryInternal(cwdPlugins);
    }
    
    Logger::error("Plugin directory does not exist: {}", directory);
    if (!exeDirPlugins.empty()) {
        Logger::error("Alternative plugin directory also does not exist: {}", exeDirPlugins);
    }
    Logger::error("Current working directory plugin directory also does not exist: {}", cwdPlugins);
    
    return false;
}

bool PluginManager::loadPluginsFromDirectoryInternal(const std::string& directory) {
    bool anyLoaded = false;
    
    // 遍历目录中的所有文件
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string extension = entry.path().extension().string();
            
            // 检查文件是否为动态库
            bool isDynamicLibrary = false;
            #ifdef _WIN32
                isDynamicLibrary = (extension == ".dll");
            #elif defined(__APPLE__)
                isDynamicLibrary = (extension == ".dylib");
            #else
                isDynamicLibrary = (extension == ".so");
            #endif
            
            if (isDynamicLibrary) {
                if (loadPlugin(filePath)) {
                    anyLoaded = true;
                }
            }
        }
    }
    
    if (!anyLoaded) {
        Logger::info("No plugins loaded from directory: {}", directory);
    }
    
    return anyLoaded;
}