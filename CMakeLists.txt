cmake_minimum_required(VERSION 3.21)
project(GLFWImGuiVulkanDemo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置CMake模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 寻找Vulkan
# 尝试从环境变量获取Vulkan SDK路径
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${VULKAN_SDK_PATH}")
    message(STATUS "Found Vulkan SDK from environment variable: ${VULKAN_SDK_PATH}")
else()
    # 尝试常见的Vulkan SDK安装路径
    set(VULKAN_SDK_PATHS
        "D:/VulkanSDK"  # 用户指定的Vulkan SDK路径
        "C:/VulkanSDK"  # 32位Windows
        "C:/Program Files/Vulkan SDK"  # 64位Windows常见路径
        "C:/Program Files (x86)/Vulkan SDK"  # 32位Windows常见路径
    )
    
    foreach(PATH ${VULKAN_SDK_PATHS})
        if(EXISTS "${PATH}")
            # 查找最新版本的Vulkan SDK
            file(GLOB SDK_VERSIONS "${PATH}/*")
            # 对目录进行排序（不使用DESCENDING选项，确保CMake版本兼容性）
            list(SORT SDK_VERSIONS)
            if(SDK_VERSIONS)
                # 反转列表以获取最新版本
                list(REVERSE SDK_VERSIONS)
                list(GET SDK_VERSIONS 0 LATEST_SDK)
                set(VULKAN_SDK_PATH "${LATEST_SDK}")
                set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${VULKAN_SDK_PATH}")
                message(STATUS "Found Vulkan SDK: ${VULKAN_SDK_PATH}")
                break()
            endif()
        endif()
    endforeach()
endif()

# 寻找Vulkan库
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message(STATUS "Vulkan found: ${Vulkan_VERSION}")
    message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
    message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")
else()
    message(FATAL_ERROR "Vulkan SDK not found! Please install Vulkan SDK from https://vulkan.lunarg.com/ and set VULKAN_SDK environment variable.")
endif()

# 使用FetchContent获取GLFW
include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
FetchContent_MakeAvailable(glfw)

# 获取ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.9
)

# 获取GLM数学库
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)

FetchContent_MakeAvailable(imgui glm)

# ImGui Vulkan后端所需的文件
set(IMGUI_SOURCE_FILES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

# 项目源文件
set(PROJECT_SOURCE_FILES
    src/main.cpp
    src/core/Application.cpp
    src/camera/Camera.cpp
    src/input/InputHandler.cpp
    src/render/Renderer.cpp
    src/vulkan/VulkanContext.cpp
    src/ui/UI.cpp
)

# 创建可执行文件
add_executable(demo ${PROJECT_SOURCE_FILES} ${IMGUI_SOURCE_FILES})

# 包含目录
target_include_directories(demo PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${glfw_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${glm_SOURCE_DIR}
    src/core
    src/camera
    src/input
    src/render
    src/vulkan
    src/ui
)

# 链接库
target_link_libraries(demo PRIVATE
    ${Vulkan_LIBRARIES}
    glfw
    glm
)

# 平台特定设置
if(WIN32)
    target_compile_definitions(demo PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()
