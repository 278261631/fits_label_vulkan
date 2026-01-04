#include "Renderer.h"
#include "UI.h"
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
    std::cout << "  Entering drawFrame..." << std::endl;
    
    VkDevice device = m_vulkanContext->getDevice();
    const std::vector<VkFence>& inFlightFences = m_vulkanContext->getInFlightFences();
    size_t currentFrame = m_vulkanContext->getCurrentFrame();
    
    std::cout << "  Current frame: " << currentFrame << std::endl;

    std::cout << "  Calling vkWaitForFences..." << std::endl;
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    std::cout << "  vkWaitForFences succeeded!" << std::endl;

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        device,
        m_vulkanContext->getSwapchain(),
        UINT64_MAX,
        m_vulkanContext->getImageAvailableSemaphores()[currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );
    
    std::cout << "  vkAcquireNextImageKHR result: " << result << std::endl;

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    std::cout << "  Calling vkResetFences..." << std::endl;
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    std::cout << "  vkResetFences succeeded!" << std::endl;
    
    std::cout << "  Calling vkResetCommandBuffer..." << std::endl;
    vkResetCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame], 0);
    std::cout << "  vkResetCommandBuffer succeeded!" << std::endl;

    // 开始录制命令缓冲区
    std::cout << "  Calling vkBeginCommandBuffer..." << std::endl;
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin command buffer recording!");
    }
    std::cout << "  vkBeginCommandBuffer succeeded!" << std::endl;

    // 绑定帧缓冲区
    std::cout << "  Binding framebuffer..." << std::endl;
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
    
    // 注意：UI渲染会在结束渲染通道后进行，所以这里不调用drawCoordinateSystem和drawGrid
    
    // 结束渲染通道
    std::cout << "  Calling vkCmdEndRenderPass..." << std::endl;
    vkCmdEndRenderPass(m_vulkanContext->getCommandBuffers()[currentFrame]);
    
    // 检查是否有UI需要更新和渲染
    if (m_ui != nullptr) {
        std::cout << "  Calling UI::update()..." << std::endl;
        m_ui->update();
        
        // 执行ImGui渲染命令
        std::cout << "  Calling ImGui_ImplVulkan_RenderDrawData..." << std::endl;
        ImDrawData* drawData = ImGui::GetDrawData();
        if (drawData && drawData->CmdListsCount > 0) {
            ImGui_ImplVulkan_RenderDrawData(drawData, m_vulkanContext->getCommandBuffers()[currentFrame]);
        }
        std::cout << "  ImGui_ImplVulkan_RenderDrawData succeeded!" << std::endl;
    } else {
        std::cout << "  No UI to update, skipping..." << std::endl;
    }
    
    // 结束录制命令缓冲区
    std::cout << "  Calling vkEndCommandBuffer..." << std::endl;
    if (vkEndCommandBuffer(m_vulkanContext->getCommandBuffers()[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer recording!");
    }
    std::cout << "  vkEndCommandBuffer succeeded!" << std::endl;

    std::cout << "  Creating submit info..." << std::endl;
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

    std::cout << "  Calling vkQueueSubmit..." << std::endl;
    if (vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }
    std::cout << "  vkQueueSubmit succeeded!" << std::endl;

    std::cout << "  Creating present info..." << std::endl;
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_vulkanContext->getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    std::cout << "  Calling vkQueuePresentKHR..." << std::endl;
    result = vkQueuePresentKHR(m_vulkanContext->getPresentQueue(), &presentInfo);
    std::cout << "  vkQueuePresentKHR result: " << result << std::endl;

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    std::cout << "  Calling nextFrame..." << std::endl;
    m_vulkanContext->nextFrame();
    std::cout << "  nextFrame succeeded!" << std::endl;
    
    std::cout << "  Exiting drawFrame..." << std::endl;
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
