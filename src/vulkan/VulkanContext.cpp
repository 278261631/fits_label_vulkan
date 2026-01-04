#include "VulkanContext.h"
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

VulkanContext::VulkanContext(int width, int height, const char* title)
    : m_width(width), m_height(height), m_title(title),
      m_window(nullptr), m_instance(VK_NULL_HANDLE),
      m_physicalDevice(VK_NULL_HANDLE), m_device(VK_NULL_HANDLE),
      m_graphicsQueue(VK_NULL_HANDLE), m_presentQueue(VK_NULL_HANDLE),
      m_surface(VK_NULL_HANDLE), m_swapchain(VK_NULL_HANDLE),
      m_swapchainImageFormat(VK_FORMAT_UNDEFINED),
      m_swapchainExtent({0, 0}),
      m_renderPass(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE),
      m_graphicsPipeline(VK_NULL_HANDLE), m_commandPool(VK_NULL_HANDLE),
      m_currentFrame(0), m_framebufferResized(false) {}

VulkanContext::~VulkanContext() {
    cleanup();
}

bool VulkanContext::init() {
    if (!initWindow()) {
        return false;
    }

    if (!initVulkan()) {
        cleanup();
        return false;
    }

    return true;
}

bool VulkanContext::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        return false;
    }

    return true;
}

bool VulkanContext::initVulkan() {
    try {
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        std::cout << "Completed createLogicalDevice!" << std::endl;
        
        createSwapchain();
        std::cout << "Completed createSwapchain!" << std::endl;
        
        createImageViews();
        std::cout << "Completed createImageViews!" << std::endl;
        
        createRenderPass();
        std::cout << "Completed createRenderPass!" << std::endl;
        
        createGraphicsPipeline();
        std::cout << "Completed createGraphicsPipeline!" << std::endl;
        
        createFramebuffers();
        std::cout << "Completed createFramebuffers!" << std::endl;
        
        createCommandPool();
        std::cout << "Completed createCommandPool!" << std::endl;
        
        createCommandBuffers();
        std::cout << "Completed createCommandBuffers!" << std::endl;
        
        createSyncObjects();
        std::cout << "Completed createSyncObjects!" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Vulkan initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void VulkanContext::createInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Demo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void VulkanContext::createSurface() {
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void VulkanContext::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // 选择第一个支持所需队列族的设备
    for (const auto& device : devices) {
        QueueFamilyIndices indices = findQueueFamilies(device, m_surface);
        if (indices.isComplete()) {
            m_physicalDevice = device;
            
            // 打印设备名称
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProps);
            std::cout << "Selected GPU: " << deviceProps.deviceName << std::endl;
            return;
        }
    }

    throw std::runtime_error("Failed to find a GPU with required queue families!");
}

void VulkanContext::createLogicalDevice() {
    std::cout << "Entering createLogicalDevice..." << std::endl;
    
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_surface);
    
    std::cout << "Got queue family indices..." << std::endl;

    // 检查队列族是否存在
    if (!indices.graphicsFamily.has_value() || !indices.presentFamily.has_value()) {
        throw std::runtime_error("Failed to find required queue families!");
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    
    // 直接使用两个队列族，不使用std::set去重
    uint32_t graphicsFamily = indices.graphicsFamily.value();
    uint32_t presentFamily = indices.presentFamily.value();
    
    std::cout << "Graphics family index: " << graphicsFamily << std::endl;
    std::cout << "Present family index: " << presentFamily << std::endl;
    
    float queuePriority = 1.0f;
    
    // 添加图形队列
    VkDeviceQueueCreateInfo graphicsQueueInfo{};
    graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.queueFamilyIndex = graphicsFamily;
    graphicsQueueInfo.queueCount = 1;
    graphicsQueueInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(graphicsQueueInfo);
    
    // 如果呈现队列族不同，添加呈现队列
    if (graphicsFamily != presentFamily) {
        VkDeviceQueueCreateInfo presentQueueInfo{};
        presentQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueInfo.queueFamilyIndex = presentFamily;
        presentQueueInfo.queueCount = 1;
        presentQueueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(presentQueueInfo);
    }
    
    std::cout << "Created queue create infos, size: " << queueCreateInfos.size() << std::endl;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;
    
    std::cout << "Calling vkCreateDevice..." << std::endl;

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }
    
    std::cout << "vkCreateDevice succeeded!" << std::endl;

    std::cout << "Calling vkGetDeviceQueue for graphics queue..." << std::endl;
    vkGetDeviceQueue(m_device, graphicsFamily, 0, &m_graphicsQueue);
    std::cout << "Calling vkGetDeviceQueue for present queue..." << std::endl;
    vkGetDeviceQueue(m_device, presentFamily, 0, &m_presentQueue);
    
    std::cout << "createLogicalDevice completed successfully!" << std::endl;
}

void VulkanContext::createSwapchain() {
    // 简化的交换链创建，实际项目中应该更完整
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount = 2;
    createInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) };
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    m_swapchainExtent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) };
}

void VulkanContext::createImageViews() {
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void VulkanContext::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void VulkanContext::createGraphicsPipeline() {
    // 简化的管线创建，实际项目中应该更完整
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // 这里应该创建完整的图形管线，包括着色器等
    // 由于我们主要使用ImGui渲染，这里简化处理
    m_graphicsPipeline = VK_NULL_HANDLE;
}

void VulkanContext::createFramebuffers() {
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffers!");
        }
    }
}

void VulkanContext::createCommandPool() {
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice, m_surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void VulkanContext::createCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void VulkanContext::createSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create sync objects!");
        }
    }
}

bool VulkanContext::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void VulkanContext::pollEvents() {
    glfwPollEvents();
}

void VulkanContext::cleanup() {
    // 清理Vulkan资源
    for (size_t i = 0; i < m_swapchainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(m_device, m_swapchainFramebuffers[i], nullptr);
    }

    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    }

    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    // 清理窗口
    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}
