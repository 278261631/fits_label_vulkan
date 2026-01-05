#include "Renderer.h"
#include "UI.h"
#include "Config.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(VulkanContext* vulkanContext, Camera* camera, UI* ui)
    : m_vulkanContext(vulkanContext), m_camera(camera), m_ui(ui),
      m_descriptorPool(VK_NULL_HANDLE) {}

Renderer::~Renderer() {
    cleanup();
}

bool Renderer::init() {
    std::cout << "Entering Renderer::init..." << std::endl;
    
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

        std::cout << "Got descriptor pool sizes..." << std::endl;
        
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        
        std::cout << "Created descriptor pool create info..." << std::endl;

        VkDevice device = m_vulkanContext->getDevice();
        std::cout << "Got device: " << device << std::endl;
        
        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
        
        std::cout << "vkCreateDescriptorPool succeeded!" << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Renderer initialization error: " << e.what() << std::endl;
        cleanup();
        return false;
    }
}

void Renderer::render() {
    drawFrame();
}

void Renderer::drawFrame() {
    Config& config = Config::getInstance();
    
    if (config.isDebugMode()) {
        std::cout << "  Entering drawFrame..." << std::endl;
    }
    
    VkDevice device = m_vulkanContext->getDevice();
    const std::vector<VkFence>& inFlightFences = m_vulkanContext->getInFlightFences();
    size_t currentFrame = m_vulkanContext->getCurrentFrame();
    
    if (config.isDebugMode()) {
        std::cout << "  Current frame: " << currentFrame << std::endl;
    }

    if (config.isDebugMode()) {
        std::cout << "  Calling vkWaitForFences..." << std::endl;
    }
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    if (config.isDebugMode()) {
        std::cout << "  vkWaitForFences succeeded!" << std::endl;
    }

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device,
        m_vulkanContext->getSwapchain(),
        UINT64_MAX,
        m_vulkanContext->getImageAvailableSemaphores()[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );
    
    if (config.isDebugMode()) {
        std::cout << "  vkAcquireNextImageKHR result: " << result << std::endl;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    if (config.isDebugMode()) {
        std::cout << "  Calling vkResetFences..." << std::endl;
    }
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    if (config.isDebugMode()) {
        std::cout << "  vkResetFences succeeded!" << std::endl;
    }
    
    if (config.isDebugMode()) {
        std::cout << "  Calling vkResetCommandBuffer..." << std::endl;
    }
    vkResetCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame], 0);
    if (config.isDebugMode()) {
        std::cout << "  vkResetCommandBuffer succeeded!" << std::endl;
    }

    // 开始录制命令缓冲区
    if (config.isDebugMode()) {
        std::cout << "  Calling vkBeginCommandBuffer..." << std::endl;
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer recording!");
    }
    if (config.isDebugMode()) {
        std::cout << "  vkBeginCommandBuffer succeeded!" << std::endl;
    }

    // 绑定帧缓冲区
    if (config.isDebugMode()) {
        std::cout << "  Binding framebuffer..." << std::endl;
    }
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
    if (config.isDebugMode()) {
        std::cout << "  Drawing simple coordinate system..." << std::endl;
    }
    
    // 注意：为了稳定运行，我们暂时只使用背景色，不做复杂的坐标绘制
    // 复杂的坐标系绘制需要完整的Vulkan管线和顶点缓冲设置
    // 后续可以添加完整的坐标系绘制功能，但需要更复杂的实现
    
    // 检查是否有UI需要更新和渲染
    if (m_ui != nullptr) {
        if (config.isDebugMode()) {
            std::cout << "  Calling UI::update()..." << std::endl;
        }
        m_ui->update();
        
        // 执行ImGui渲染命令 - 注意：这必须在渲染通道内进行！
        if (config.isDebugMode()) {
            std::cout << "  Calling ImGui_ImplVulkan_RenderDrawData..." << std::endl;
        }
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData && drawData->CmdListsCount > 0) {
            ImGui_ImplVulkan_RenderDrawData(drawData, m_vulkanContext->getCommandBuffers()[currentFrame]);
        }
        if (config.isDebugMode()) {
            std::cout << "  ImGui_ImplVulkan_RenderDrawData succeeded!" << std::endl;
        }
    } else {
        if (config.isDebugMode()) {
            std::cout << "  No UI to update, skipping..." << std::endl;
        }
    }
    
    // 结束渲染通道
    if (config.isDebugMode()) {
        std::cout << "  Calling vkCmdEndRenderPass..." << std::endl;
    }
    vkCmdEndRenderPass(m_vulkanContext->getCommandBuffers()[currentFrame]);
    
    // 结束录制命令缓冲区
    if (config.isDebugMode()) {
        std::cout << "  Calling vkEndCommandBuffer..." << std::endl;
    }
    if (vkEndCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }
    if (config.isDebugMode()) {
        std::cout << "  vkEndCommandBuffer succeeded!" << std::endl;
    }

    if (config.isDebugMode()) {
        std::cout << "  Creating submit info..." << std::endl;
    }
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

    if (config.isDebugMode()) {
        std::cout << "  Calling vkQueueSubmit..." << std::endl;
    }
    if (vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    if (config.isDebugMode()) {
        std::cout << "  vkQueueSubmit succeeded!" << std::endl;
    }

    if (config.isDebugMode()) {
        std::cout << "  Creating present info..." << std::endl;
    }
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_vulkanContext->getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    if (config.isDebugMode()) {
        std::cout << "  Calling vkQueuePresentKHR..." << std::endl;
    }
    result = vkQueuePresentKHR(m_vulkanContext->getPresentQueue(), &presentInfo);
    if (config.isDebugMode()) {
        std::cout << "  vkQueuePresentKHR result: " << result << std::endl;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    if (config.isDebugMode()) {
        std::cout << "  Calling nextFrame..." << std::endl;
    }
    m_vulkanContext->nextFrame();
    if (config.isDebugMode()) {
        std::cout << "  nextFrame succeeded!" << std::endl;
    }
    
    if (config.isDebugMode()) {
        std::cout << "  Exiting drawFrame..." << std::endl;
    }
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
