#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>

class VulkanContext {
public:
    VulkanContext(int width, int height, const char* title);
    ~VulkanContext();

    bool init();
    void cleanup();
    bool shouldClose() const;
    void pollEvents();

    // Getters
    GLFWwindow* getWindow() const { return m_window; }
    VkInstance getInstance() const { return m_instance; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkDevice getDevice() const { return m_device; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getPresentQueue() const { return m_presentQueue; }
    VkSurfaceKHR getSurface() const { return m_surface; }
    VkSwapchainKHR getSwapchain() const { return m_swapchain; }
    const std::vector<VkImage>& getSwapchainImages() const { return m_swapchainImages; }
    VkFormat getSwapchainImageFormat() const { return m_swapchainImageFormat; }
    VkExtent2D getSwapchainExtent() const { return m_swapchainExtent; }
    const std::vector<VkImageView>& getSwapchainImageViews() const { return m_swapchainImageViews; }
    VkRenderPass getRenderPass() const { return m_renderPass; }
    const std::vector<VkFramebuffer>& getSwapchainFramebuffers() const { return m_swapchainFramebuffers; }
    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
    VkPipeline getGraphicsPipeline() const { return m_graphicsPipeline; }
    const std::vector<VkCommandBuffer>& getCommandBuffers() const { return m_commandBuffers; }
    const std::vector<VkSemaphore>& getImageAvailableSemaphores() const { return m_imageAvailableSemaphores; }
    const std::vector<VkSemaphore>& getRenderFinishedSemaphores() const { return m_renderFinishedSemaphores; }
    const std::vector<VkFence>& getInFlightFences() const { return m_inFlightFences; }
    size_t getCurrentFrame() const { return m_currentFrame; }
    void nextFrame() { m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; }

private:
    bool initWindow();
    bool initVulkan();
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    static const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    // Window
    int m_width;
    int m_height;
    const char* m_title;
    GLFWwindow* m_window;

    // Vulkan
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    VkSurfaceKHR m_surface;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainImages;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImageView> m_swapchainImageViews;
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    size_t m_currentFrame;
    bool m_framebufferResized;
};
