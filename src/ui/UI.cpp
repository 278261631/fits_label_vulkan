#include "UI.h"
#include "Renderer.h"
#include "Config.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

UI::UI(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera)
    : m_vulkanContext(vulkanContext), m_renderer(renderer), m_camera(camera),
      m_showControlPanel(true) {}

UI::~UI() {
    cleanup();
}

bool UI::init() {
    Config& config = Config::getInstance();
    
    if (config.isDebugMode()) {
        std::cout << "Entering UI::init..." << std::endl;
    }
    
    try {
        initImGui();
        if (config.isDebugMode()) {
            std::cout << "UI::init completed successfully!" << std::endl;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UI initialization error: " << e.what() << std::endl;
        cleanup();
        return false;
    }
}

void UI::initImGui() {
    // 初始化ImGui上下文
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // 设置ImGui样式
    ImGui::StyleColorsDark();

    // 初始化GLFW后端
    ImGui_ImplGlfw_InitForVulkan(m_vulkanContext->getWindow(), true);

    // 初始化Vulkan后端
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_vulkanContext->getInstance();
    init_info.PhysicalDevice = m_vulkanContext->getPhysicalDevice();
    init_info.Device = m_vulkanContext->getDevice();
    init_info.QueueFamily = 0;
    init_info.Queue = m_vulkanContext->getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_renderer->getDescriptorPool();
    init_info.Allocator = nullptr;
    init_info.RenderPass = m_vulkanContext->getRenderPass();
    init_info.MinImageCount = 2;
    init_info.ImageCount = static_cast<uint32_t>(m_vulkanContext->getSwapchainImages().size());
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info);

    // 创建ImGui字体纹理
    VkCommandPool command_pool;
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = 0;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(m_vulkanContext->getDevice(), &pool_info, nullptr, &command_pool);

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_vulkanContext->getDevice(), &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);
    vkEndCommandBuffer(command_buffer);
    
    // 提交命令缓冲区
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vulkanContext->getGraphicsQueue());
    
    // 创建ImGui字体纹理 - 不需要命令缓冲区参数
    ImGui_ImplVulkan_CreateFontsTexture();

    vkDestroyCommandPool(m_vulkanContext->getDevice(), command_pool, nullptr);
}

void UI::update() {
    Config& config = Config::getInstance();
    
    if (config.isDebugMode()) {
        std::cout << "  Entering UI::update..." << std::endl;
    }
    
    // 开始ImGui帧 - 注意顺序：先调用GLFW后端，再调用Vulkan后端
    if (config.isDebugMode()) {
        std::cout << "  Calling ImGui_ImplGlfw_NewFrame..." << std::endl;
    }
    ImGui_ImplGlfw_NewFrame();
    if (config.isDebugMode()) {
        std::cout << "  Calling ImGui_ImplVulkan_NewFrame..." << std::endl;
    }
    ImGui_ImplVulkan_NewFrame();
    if (config.isDebugMode()) {
        std::cout << "  Calling ImGui::NewFrame..." << std::endl;
    }
    ImGui::NewFrame();
    
    // 绘制坐标系和网格
    if (config.isDebugMode()) {
        std::cout << "  Drawing coordinate system..." << std::endl;
    }
    drawCoordinateSystem();
    
    // 绘制控制面板
    if (config.isDebugMode()) {
        std::cout << "  Drawing control panel..." << std::endl;
    }
    drawControlPanel();
    
    // 绘制简单的信息窗口
    if (config.isDebugMode()) {
        std::cout << "  Drawing simple ImGui text..." << std::endl;
    }
    ImGui::Begin("Simple Info");
    ImGui::Text("Hello Vulkan!");
    ImGui::End();
    
    if (config.isDebugMode()) {
        std::cout << "  Calling ImGui::Render..." << std::endl;
    }
    ImGui::Render();
    
    if (config.isDebugMode()) {
        std::cout << "  Exiting UI::update..." << std::endl;
    }
}

void UI::drawCoordinateSystem() {
    // 创建全屏窗口用于绘制坐标系
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | 
                                   ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Coordinate System", nullptr, window_flags);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    ImVec2 center = ImVec2(display_size.x * 0.5f, display_size.y * 0.5f);

    float base_axis_length = 100.0f;
    float axis_thickness = 2.0f;

    // 绘制网格
    drawGrid();

    // 定义坐标轴的3D坐标
    glm::vec3 origin(0.0f, 0.0f, 0.0f);
    glm::vec3 x_axis(base_axis_length, 0.0f, 0.0f);
    glm::vec3 y_axis(0.0f, base_axis_length, 0.0f);
    glm::vec3 z_axis(0.0f, 0.0f, base_axis_length);

    // 获取视图矩阵
    const glm::mat4& viewMatrix = m_camera->getViewMatrix();

    // 转换3D坐标到屏幕坐标
    auto toScreenPos = [&](const glm::vec3& pos) -> ImVec2 {
        // 应用视图变换
        glm::vec4 viewPos = viewMatrix * glm::vec4(pos, 1.0f);
        
        // 转换到屏幕坐标
        float screen_x = center.x + viewPos.x * 100.0f; // 缩放因子调整
        float screen_y = center.y + viewPos.y * 100.0f;
        
        return ImVec2(screen_x, screen_y);
    };

    ImVec2 origin_2d = toScreenPos(origin);
    ImVec2 x_axis_2d = toScreenPos(x_axis);
    ImVec2 y_axis_2d = toScreenPos(y_axis);
    ImVec2 z_axis_2d = toScreenPos(z_axis);

    // 绘制坐标轴
    draw_list->AddLine(origin_2d, x_axis_2d, IM_COL32(255, 0, 0, 255), axis_thickness);
    draw_list->AddLine(origin_2d, y_axis_2d, IM_COL32(0, 255, 0, 255), axis_thickness);
    draw_list->AddLine(origin_2d, z_axis_2d, IM_COL32(0, 0, 255, 255), axis_thickness);

    // 绘制箭头
    auto drawArrow = [&](const ImVec2& start, const ImVec2& end, ImU32 color, float thickness) {
        ImVec2 dir = ImVec2(end.x - start.x, end.y - start.y);
        float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
        ImVec2 unit_dir = ImVec2(dir.x / length, dir.y / length);
        ImVec2 perp_dir = ImVec2(-unit_dir.y, unit_dir.x);

        float arrow_size = 10.0f;
        ImVec2 arrow_left = ImVec2(
            end.x - unit_dir.x * arrow_size + perp_dir.x * arrow_size * 0.5f,
            end.y - unit_dir.y * arrow_size + perp_dir.y * arrow_size * 0.5f
        );
        ImVec2 arrow_right = ImVec2(
            end.x - unit_dir.x * arrow_size - perp_dir.x * arrow_size * 0.5f,
            end.y - unit_dir.y * arrow_size - perp_dir.y * arrow_size * 0.5f
        );

        draw_list->AddLine(end, arrow_left, color, thickness);
        draw_list->AddLine(end, arrow_right, color, thickness);
    };

    drawArrow(origin_2d, x_axis_2d, IM_COL32(255, 0, 0, 255), axis_thickness);
    drawArrow(origin_2d, y_axis_2d, IM_COL32(0, 255, 0, 255), axis_thickness);
    drawArrow(origin_2d, z_axis_2d, IM_COL32(0, 0, 255, 255), axis_thickness);

    // 绘制标签
    draw_list->AddText(ImVec2(x_axis_2d.x + 5.0f, x_axis_2d.y - 10.0f), IM_COL32(255, 0, 0, 255), "X");
    draw_list->AddText(ImVec2(y_axis_2d.x + 5.0f, y_axis_2d.y - 10.0f), IM_COL32(0, 255, 0, 255), "Y");
    draw_list->AddText(ImVec2(z_axis_2d.x + 5.0f, z_axis_2d.y + 5.0f), IM_COL32(0, 0, 255, 255), "Z");

    ImGui::End();
}

void UI::drawGrid() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    ImVec2 center = ImVec2(display_size.x * 0.5f, display_size.y * 0.5f);

    float grid_size = 10.0f;
    float grid_spacing = 20.0f;
    float grid_thickness = 1.0f;
    ImU32 grid_color = IM_COL32(100, 100, 100, 150);

    // 获取视图矩阵
    const glm::mat4& viewMatrix = m_camera->getViewMatrix();

    // 转换3D坐标到屏幕坐标
    auto toScreenPos = [&](const glm::vec3& pos) -> ImVec2 {
        glm::vec3 transformed = glm::vec3(viewMatrix * glm::vec4(pos, 1.0f));
        float screen_x = center.x + transformed.x;
        float screen_y = center.y + transformed.y;
        return ImVec2(screen_x, screen_y);
    };

    // 绘制xy平面网格
    for (int i = -grid_size; i <= grid_size; i++) {
        // 绘制垂直线 (x = i * grid_spacing, z = 0)
        glm::vec3 grid_start(i * grid_spacing, -grid_size * grid_spacing, 0.0f);
        glm::vec3 grid_end(i * grid_spacing, grid_size * grid_spacing, 0.0f);
        ImVec2 start_2d = toScreenPos(grid_start);
        ImVec2 end_2d = toScreenPos(grid_end);
        draw_list->AddLine(start_2d, end_2d, grid_color, grid_thickness);

        // 绘制水平线 (y = i * grid_spacing, z = 0)
        grid_start = glm::vec3(-grid_size * grid_spacing, i * grid_spacing, 0.0f);
        grid_end = glm::vec3(grid_size * grid_spacing, i * grid_spacing, 0.0f);
        start_2d = toScreenPos(grid_start);
        end_2d = toScreenPos(grid_end);
        draw_list->AddLine(start_2d, end_2d, grid_color, grid_thickness);
    }
}

void UI::drawControlPanel() {
    Config& config = Config::getInstance();
    
    // 显示控制面板
    ImGui::Begin("Control Panel", &m_showControlPanel, ImGuiWindowFlags_AlwaysAutoResize);
    
    // 相机控制
    ImGui::Text("Camera Controls");
    ImGui::Separator();
    ImGui::Text("Rotation X: %.3f", m_camera->getRotationX());
    ImGui::Text("Rotation Y: %.3f", m_camera->getRotationY());
    ImGui::Text("Zoom: %.3f", m_camera->getZoom());
    ImGui::Separator();
    
    // 配置设置
    ImGui::Text("Configuration");
    ImGui::Separator();
    
    // FPS设置
    int fps = config.getFPS();
    if (ImGui::SliderInt("Target FPS", &fps, 10, 240)) {
        config.setFPS(fps);
    }
    
    // Debug模式设置
    bool debugMode = config.isDebugMode();
    if (ImGui::Checkbox("Debug Mode", &debugMode)) {
        config.setDebugMode(debugMode);
    }
    
    ImGui::Separator();
    
    // 指令说明
    ImGui::Text("Instructions:");
    ImGui::Text("- Left Click + Drag: Rotate");
    ImGui::Text("- Middle Click + Drag: Pan");
    ImGui::Text("- Scroll: Zoom");
    ImGui::Text("- ESC: Exit");
    
    ImGui::End();
}

void UI::cleanup() {
    // 清理ImGui
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
