#include "Renderer.h"
#include "UI.h"
#include "Config.h"
#include "Logger.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(VulkanContext* vulkanContext, Camera* camera, UI* ui)
    : m_vulkanContext(vulkanContext), m_camera(camera), m_ui(ui),
      m_descriptorPool(VK_NULL_HANDLE) {}

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
    
    // 绘制简单的坐标系
    Logger::debug("  Drawing simple coordinate system...");
    
    // 注意：为了稳定运行，我们暂时只使用背景色，不做复杂的坐标绘制
    // 复杂的坐标系绘制需要完整的Vulkan管线和顶点缓冲设置
    // 后续可以添加完整的坐标系绘制功能，但需要更复杂的实现
    
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
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_vulkanContext->getDevice(), m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
}
