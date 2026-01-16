#include "Renderer.h"
#include "UI.h"
#include "CoordinateSystemRenderer.h"
#include "DemoObjectRenderer.h"
#include "GridRenderer.h"
#include "PointCloudRenderer.h"
#include "PluginContext.h"
#include "Config.h"
#include "Logger.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(VulkanContext* vulkanContext, Camera* camera, UI* ui)
    : m_vulkanContext(vulkanContext), m_camera(camera), m_ui(ui),
      m_pluginContext(nullptr), m_descriptorPool(VK_NULL_HANDLE) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init() {
    Config& config = Config::getInstance();
    
    Logger::debug("Entering Renderer::init...");
    
    try {
        // 创建ImGui描述符池
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        Logger::debug("Got descriptor pool sizes...");
        
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        
        Logger::debug("Created descriptor pool create info...");
        
        VkDevice device = m_vulkanContext->getDevice();
        Logger::debug("Got device: {:p}", static_cast<void*>(device));
        
        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
        
        Logger::debug("vkCreateDescriptorPool succeeded!");

        // 初始化坐标系渲染器
        m_coordinateRenderer = std::make_unique<CoordinateSystemRenderer>(m_vulkanContext, m_camera);
        if (!m_coordinateRenderer->init()) {
            Logger::error("Failed to initialize coordinate system renderer!");
            cleanup();
            return false;
        }
        
        // 初始化演示物体渲染器
        m_demoObjectRenderer = std::make_unique<DemoObjectRenderer>(m_vulkanContext, m_camera);
        if (!m_demoObjectRenderer->init()) {
            Logger::error("Failed to initialize demo object renderer!");
            cleanup();
            return false;
        }

        // 初始化网格渲染器
        m_gridRenderer = std::make_unique<GridRenderer>(m_vulkanContext, m_camera);
        if (!m_gridRenderer->init()) {
            Logger::error("Failed to initialize grid renderer!");
            cleanup();
            return false;
        }

        // 初始化点云渲染器
        m_pointCloudRenderer = std::make_unique<PointCloudRenderer>(m_vulkanContext, m_camera);
        if (!m_pointCloudRenderer->init()) {
            Logger::error("Failed to initialize point cloud renderer!");
            cleanup();
            return false;
        }

    return true;
    } catch (const std::exception& e) {
        Logger::error("Renderer initialization error: {}", e.what());
        cleanup();
        return false;
    }
}

void Renderer::render() {
    drawFrame();
}

void Renderer::drawFrame() {
    Config& config = Config::getInstance();
    
    Logger::debug("  Entering drawFrame...");
    
    VkDevice device = m_vulkanContext->getDevice();
    const std::vector<VkFence>& inFlightFences = m_vulkanContext->getInFlightFences();
    size_t currentFrame = m_vulkanContext->getCurrentFrame();
    
    Logger::debug("  Current frame: {}", currentFrame);

    Logger::debug("  Calling vkWaitForFences...");
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    Logger::debug("  vkWaitForFences succeeded!");

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device,
        m_vulkanContext->getSwapchain(),
        UINT64_MAX,
        m_vulkanContext->getImageAvailableSemaphores()[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );
    
    Logger::debug("  vkAcquireNextImageKHR result: {}", static_cast<int>(result));

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    Logger::debug("  Calling vkResetFences...");
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    Logger::debug("  vkResetFences succeeded!");
    
    Logger::debug("  Calling vkResetCommandBuffer...");
    vkResetCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame], 0);
    Logger::debug("  vkResetCommandBuffer succeeded!");

    // 开始录制命令缓冲区
    Logger::debug("  Calling vkBeginCommandBuffer...");
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer recording!");
    }
    Logger::debug("  vkBeginCommandBuffer succeeded!");

    // 绑定帧缓冲区
    Logger::debug("  Binding framebuffer...");
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_vulkanContext->getRenderPass();
    renderPassInfo.framebuffer = m_vulkanContext->getSwapchainFramebuffers()[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_vulkanContext->getSwapchainExtent();
    
    VkClearValue clearColor = { {{0.1f, 0.1f, 0.1f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(m_vulkanContext->getCommandBuffers()[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 绘制网格（先绘制，作为背景） - DISABLED
    // Logger::debug("  Drawing grid...");
    // if (m_gridRenderer) {
    //     m_gridRenderer->draw(m_vulkanContext->getCommandBuffers()[currentFrame]);
    // }

    // 绘制演示物体 - DISABLED
    // Logger::debug("  Drawing demo object...");
    // if (m_demoObjectRenderer) {
    //     m_demoObjectRenderer->draw(m_vulkanContext->getCommandBuffers()[currentFrame]);
    // }

    // 绘制坐标系 - DISABLED
    // Logger::debug("  Drawing coordinate system...");
    // if (m_coordinateRenderer) {
    //     m_coordinateRenderer->draw(m_vulkanContext->getCommandBuffers()[currentFrame]);
    // }

    // 绘制点云
    Logger::debug("  Drawing point cloud...");
    if (m_pointCloudRenderer && m_pluginContext) {
        m_pointCloudRenderer->updatePoints(m_pluginContext);
        m_pointCloudRenderer->draw(m_vulkanContext->getCommandBuffers()[currentFrame]);
    }

    // 检查是否有UI需要更新和渲染
    if (m_ui != nullptr) {
        Logger::debug("  Calling UI::update()...");
        m_ui->update();

        // 执行ImGui渲染命令 - 注意：这必须在渲染通道内进行！
        Logger::debug("  Calling ImGui_ImplVulkan_RenderDrawData...");
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData && drawData->CmdListsCount > 0) {
            ImGui_ImplVulkan_RenderDrawData(drawData, m_vulkanContext->getCommandBuffers()[currentFrame]);
        }
        Logger::debug("  ImGui_ImplVulkan_RenderDrawData succeeded!");
    } else {
        Logger::debug("  No UI to update, skipping...");
    }
    
    // 结束渲染通道
    Logger::debug("  Calling vkCmdEndRenderPass...");
    vkCmdEndRenderPass(m_vulkanContext->getCommandBuffers()[currentFrame]);
    
    // 结束录制命令缓冲区
    Logger::debug("  Calling vkEndCommandBuffer...");
    if (vkEndCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }
    Logger::debug("  vkEndCommandBuffer succeeded!");

    Logger::debug("  Creating submit info...");
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_vulkanContext->getImageAvailableSemaphores()[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vulkanContext->getCommandBuffers()[currentFrame];

    VkSemaphore signalSemaphores[] = { m_vulkanContext->getRenderFinishedSemaphores()[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    Logger::debug("  Calling vkQueueSubmit...");
    if (vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    Logger::debug("  vkQueueSubmit succeeded!");

    Logger::debug("  Creating present info...");
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_vulkanContext->getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    Logger::debug("  Calling vkQueuePresentKHR...");
    result = vkQueuePresentKHR(m_vulkanContext->getPresentQueue(), &presentInfo);
    Logger::debug("  vkQueuePresentKHR result: {}", static_cast<int>(result));

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    Logger::debug("  Calling nextFrame...");
    m_vulkanContext->nextFrame();
    Logger::debug("  nextFrame succeeded!");
    
    Logger::debug("  Exiting drawFrame...");
}

void Renderer::drawCoordinateSystem() {
    // 这个函数将在UI类中实现，使用ImGui绘制坐标系
}

void Renderer::drawGrid() {
    // 这个函数将在UI类中实现，使用ImGui绘制网格
}

void Renderer::cleanup() {
    // 清理坐标系渲染器
    if (m_coordinateRenderer) {
        m_coordinateRenderer->cleanup();
        m_coordinateRenderer.reset();
    }

    // 清理演示物体渲染器
    if (m_demoObjectRenderer) {
        m_demoObjectRenderer->cleanup();
        m_demoObjectRenderer.reset();
    }

    // 清理网格渲染器
    if (m_gridRenderer) {
        m_gridRenderer->cleanup();
        m_gridRenderer.reset();
    }

    // 清理点云渲染器
    if (m_pointCloudRenderer) {
        m_pointCloudRenderer->cleanup();
        m_pointCloudRenderer.reset();
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_vulkanContext->getDevice(), m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
}
