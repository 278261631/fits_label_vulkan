#include "DemoObjectRenderer.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "Logger.h"
#include "ShaderCompiler.h"
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <array>
#include <glm/gtc/matrix_transform.hpp>

DemoObjectRenderer::DemoObjectRenderer(VulkanContext* vulkanContext, Camera* camera)
    : m_vulkanContext(vulkanContext), m_camera(camera),
      m_vertexBuffer(VK_NULL_HANDLE), m_vertexBufferMemory(VK_NULL_HANDLE),
      m_indexBuffer(VK_NULL_HANDLE), m_indexBufferMemory(VK_NULL_HANDLE),
      m_vertexShaderModule(VK_NULL_HANDLE), m_fragmentShaderModule(VK_NULL_HANDLE),
      m_pipelineLayout(VK_NULL_HANDLE), m_graphicsPipeline(VK_NULL_HANDLE),
      m_descriptorSetLayout(VK_NULL_HANDLE), m_descriptorPool(VK_NULL_HANDLE),
      m_descriptorSet(VK_NULL_HANDLE),
      m_uniformBuffer(VK_NULL_HANDLE), m_uniformBufferMemory(VK_NULL_HANDLE) {
}

DemoObjectRenderer::~DemoObjectRenderer() {
    cleanup();
}

bool DemoObjectRenderer::init() {
    try {
        generateVertexData();
        createVertexBuffer();
        createIndexBuffer();
        createShaderModules();
        createDescriptorSetLayout();
        createPipelineLayout();
        createGraphicsPipeline();
        createDescriptorPool();
        createDescriptorSets();
        
        Logger::info("DemoObjectRenderer initialized successfully!");
        return true;
    } catch (const std::exception& e) {
        Logger::error("DemoObjectRenderer initialization error: {}", e.what());
        cleanup();
        return false;
    }
}

void DemoObjectRenderer::generateVertexData() {
    Logger::debug("Generating demo object vertex data...");
    
    // 清除现有数据
    m_vertices.clear();
    m_indices.clear();
    
    // 创建一个更复杂的对象 - 一个带锥体的立方体，类似3ds Max中的基本几何体
    // 首先创建立方体部分
    std::vector<glm::vec3> cubeVertices = {
        // 前面
        glm::vec3(-0.5f, -0.5f,  0.5f),  // 0
        glm::vec3( 0.5f, -0.5f,  0.5f),  // 1
        glm::vec3( 0.5f,  0.5f,  0.5f),  // 2
        glm::vec3(-0.5f,  0.5f,  0.5f),  // 3
        // 后面
        glm::vec3(-0.5f, -0.5f, -0.5f),  // 4
        glm::vec3(-0.5f,  0.5f, -0.5f),  // 5
        glm::vec3( 0.5f,  0.5f, -0.5f),  // 6
        glm::vec3( 0.5f, -0.5f, -0.5f),  // 7
        // 顶面
        glm::vec3(-0.5f,  0.5f, -0.5f),  // 8
        glm::vec3(-0.5f,  0.5f,  0.5f),  // 9
        glm::vec3( 0.5f,  0.5f,  0.5f),  // 10
        glm::vec3( 0.5f,  0.5f, -0.5f),  // 11
        // 底面
        glm::vec3(-0.5f, -0.5f, -0.5f),  // 12
        glm::vec3( 0.5f, -0.5f, -0.5f),  // 13
        glm::vec3( 0.5f, -0.5f,  0.5f),  // 14
        glm::vec3(-0.5f, -0.5f,  0.5f),  // 15
        // 右面
        glm::vec3( 0.5f, -0.5f, -0.5f),  // 16
        glm::vec3( 0.5f,  0.5f, -0.5f),  // 17
        glm::vec3( 0.5f,  0.5f,  0.5f),  // 18
        glm::vec3( 0.5f, -0.5f,  0.5f),  // 19
        // 左面
        glm::vec3(-0.5f, -0.5f, -0.5f),  // 20
        glm::vec3(-0.5f, -0.5f,  0.5f),  // 21
        glm::vec3(-0.5f,  0.5f,  0.5f),  // 22
        glm::vec3(-0.5f,  0.5f, -0.5f),  // 23
    };
    
    // 定义立方体的索引
    std::vector<uint32_t> cubeIndices = {
        // 前面
        0, 1, 2, 2, 3, 0,
        // 后面
        4, 5, 6, 6, 7, 4,
        // 顶面
        8, 9, 10, 10, 11, 8,
        // 底面
        12, 13, 14, 14, 15, 12,
        // 右面
        16, 17, 18, 18, 19, 16,
        // 左面
        20, 21, 22, 22, 23, 20
    };
    
    // 计算立方体的法线
    std::vector<glm::vec3> cubeNormals;
    for (size_t i = 0; i < cubeVertices.size(); i++) {
        cubeNormals.push_back(glm::vec3(0.0f, 0.0f, 0.0f)); // 初始化为零向量
    }
    
    // 为每个面计算法线
    for (size_t i = 0; i < cubeIndices.size(); i += 3) {
        uint32_t idx0 = cubeIndices[i];
        uint32_t idx1 = cubeIndices[i + 1];
        uint32_t idx2 = cubeIndices[i + 2];
        
        glm::vec3 v0 = cubeVertices[idx0];
        glm::vec3 v1 = cubeVertices[idx1];
        glm::vec3 v2 = cubeVertices[idx2];
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        // 累加法线到对应的顶点
        cubeNormals[idx0] += normal;
        cubeNormals[idx1] += normal;
        cubeNormals[idx2] += normal;
    }
    
    // 归一化法线
    for (auto& normal : cubeNormals) {
        normal = glm::normalize(normal);
    }
    
    // 添加颜色信息 - 为每个面使用不同颜色
    std::vector<glm::vec3> cubeColors;
    for (size_t i = 0; i < cubeVertices.size(); i++) {
        // 根据顶点位置分配颜色
        glm::vec3 pos = cubeVertices[i];
        if (pos.z > 0.4f) { // 前面 - 红色
            cubeColors.push_back(glm::vec3(1.0f, 0.3f, 0.3f));
        } else if (pos.z < -0.4f) { // 后面 - 蓝色
            cubeColors.push_back(glm::vec3(0.3f, 0.3f, 1.0f));
        } else if (pos.y > 0.4f) { // 顶面 - 黄色
            cubeColors.push_back(glm::vec3(1.0f, 1.0f, 0.3f));
        } else if (pos.y < -0.4f) { // 底面 - 青色
            cubeColors.push_back(glm::vec3(0.3f, 1.0f, 1.0f));
        } else if (pos.x > 0.4f) { // 右面 - 绿色
            cubeColors.push_back(glm::vec3(0.3f, 1.0f, 0.3f));
        } else { // 左面 - 紫色
            cubeColors.push_back(glm::vec3(1.0f, 0.3f, 1.0f));
        }
    }
    
    // 将立方体数据添加到顶点和索引数组
    for (size_t i = 0; i < cubeVertices.size(); i++) {
        m_vertices.push_back({cubeVertices[i], cubeColors[i], cubeNormals[i]});
    }
    
    // 添加立方体索引，需要偏移顶点索引
    uint32_t vertexOffset = 0;
    for (uint32_t index : cubeIndices) {
        m_indices.push_back(index + vertexOffset);
    }
    
    // 添加一个锥体到立方体顶部中央
    glm::vec3 pyramidTop(0.0f, 0.75f, 0.0f); // 锥体顶点
    glm::vec3 baseCenter(0.0f, 0.5f, 0.0f);   // 锥体底部中心
    float baseSize = 0.3f; // 锥体底部大小
    
    // 锥体底部的四个顶点
    std::vector<glm::vec3> pyramidBaseVertices = {
        baseCenter + glm::vec3(-baseSize, 0.0f, -baseSize),  // 0
        baseCenter + glm::vec3( baseSize, 0.0f, -baseSize),  // 1
        baseCenter + glm::vec3( baseSize, 0.0f,  baseSize),  // 2
        baseCenter + glm::vec3(-baseSize, 0.0f,  baseSize)   // 3
    };
    
    // 添加锥体顶点到顶点数组
    uint32_t pyramidTopIndex = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back({pyramidTop, glm::vec3(0.8f, 0.6f, 0.2f), glm::vec3(0.0f, 1.0f, 0.0f)}); // 金色顶点
    
    // 添加锥体底部顶点
    uint32_t pyramidBaseStartIndex = static_cast<uint32_t>(m_vertices.size());
    for (const auto& vertex : pyramidBaseVertices) {
        m_vertices.push_back({vertex, glm::vec3(0.8f, 0.6f, 0.2f), glm::vec3(0.0f, -1.0f, 0.0f)}); // 底面法线向下
    }
    
    // 添加锥体侧面（四个三角形，每个三角形连接顶点和底边）
    // 侧面三角形：顶点 -> 底边顶点1 -> 底边顶点2
    m_indices.push_back(pyramidTopIndex); // 顶点
    m_indices.push_back(pyramidBaseStartIndex + 0); // 底边顶点0
    m_indices.push_back(pyramidBaseStartIndex + 1); // 底边顶点1
    
    m_indices.push_back(pyramidTopIndex); // 顶点
    m_indices.push_back(pyramidBaseStartIndex + 1); // 底边顶点1
    m_indices.push_back(pyramidBaseStartIndex + 2); // 底边顶点2
    
    m_indices.push_back(pyramidTopIndex); // 顶点
    m_indices.push_back(pyramidBaseStartIndex + 2); // 底边顶点2
    m_indices.push_back(pyramidBaseStartIndex + 3); // 底边顶点3
    
    m_indices.push_back(pyramidTopIndex); // 顶点
    m_indices.push_back(pyramidBaseStartIndex + 3); // 底边顶点3
    m_indices.push_back(pyramidBaseStartIndex + 0); // 底边顶点0
    
    // 添加锥体底面（两个三角形）
    m_indices.push_back(pyramidBaseStartIndex + 0); // 底边顶点0
    m_indices.push_back(pyramidBaseStartIndex + 3); // 底边顶点3
    m_indices.push_back(pyramidBaseStartIndex + 2); // 底边顶点2
    
    m_indices.push_back(pyramidBaseStartIndex + 0); // 底边顶点0
    m_indices.push_back(pyramidBaseStartIndex + 2); // 底边顶点2
    m_indices.push_back(pyramidBaseStartIndex + 1); // 底边顶点1
    
    Logger::debug("Generated {} vertices and {} indices for demo object", 
                 m_vertices.size(), m_indices.size());
}

void DemoObjectRenderer::createVertexBuffer() {
    Logger::debug("Creating demo object vertex buffer...");
    
    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();
    
    // 创建临时缓冲区用于复制数据
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_vulkanContext->getDevice(), &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_vulkanContext->getDevice(), stagingBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_vulkanContext->getDevice(), &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(m_vulkanContext->getDevice(), stagingBuffer, stagingBufferMemory, 0);
    
    // 将顶点数据复制到缓冲区
    void* data;
    vkMapMemory(m_vulkanContext->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_vulkanContext->getDevice(), stagingBufferMemory);
    
    // 创建顶点缓冲区
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    
    if (vkCreateBuffer(m_vulkanContext->getDevice(), &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }
    
    vkGetBufferMemoryRequirements(m_vulkanContext->getDevice(), m_vertexBuffer, &memRequirements);
    
    // 使用设备本地内存，适合GPU访问
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(m_vulkanContext->getDevice(), &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }
    
    vkBindBufferMemory(m_vulkanContext->getDevice(), m_vertexBuffer, m_vertexBufferMemory, 0);
    
    // 复制数据到顶点缓冲区
    VkCommandBuffer commandBuffer;
    
    // 简化的命令缓冲区创建（实际项目中应该使用现有的命令池）
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0;
    
    VkCommandPool commandPool;
    vkCreateCommandPool(m_vulkanContext->getDevice(), &poolInfo, nullptr, &commandPool);
    
    VkCommandBufferAllocateInfo allocInfoCmd{};
    allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfoCmd.commandPool = commandPool;
    allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandBufferCount = 1;
    
    vkAllocateCommandBuffers(m_vulkanContext->getDevice(), &allocInfoCmd, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_vertexBuffer, 1, &copyRegion);
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    // 使用图形队列而不是设备
    vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vulkanContext->getGraphicsQueue());
    
    // 清理临时资源
    vkDestroyBuffer(m_vulkanContext->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_vulkanContext->getDevice(), stagingBufferMemory, nullptr);
    vkDestroyCommandPool(m_vulkanContext->getDevice(), commandPool, nullptr);
    
    Logger::debug("Demo object vertex buffer created successfully");
}

void DemoObjectRenderer::createIndexBuffer() {
    Logger::debug("Creating demo object index buffer...");
    
    VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();
    
    // 创建临时缓冲区用于复制数据
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_vulkanContext->getDevice(), &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_vulkanContext->getDevice(), stagingBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_vulkanContext->getDevice(), &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(m_vulkanContext->getDevice(), stagingBuffer, stagingBufferMemory, 0);
    
    // 将索引数据复制到缓冲区
    void* data;
    vkMapMemory(m_vulkanContext->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_indices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_vulkanContext->getDevice(), stagingBufferMemory);
    
    // 创建索引缓冲区
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    
    if (vkCreateBuffer(m_vulkanContext->getDevice(), &bufferInfo, nullptr, &m_indexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create index buffer!");
    }
    
    vkGetBufferMemoryRequirements(m_vulkanContext->getDevice(), m_indexBuffer, &memRequirements);
    
    // 使用设备本地内存，适合GPU访问
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(m_vulkanContext->getDevice(), &allocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate index buffer memory!");
    }
    
    vkBindBufferMemory(m_vulkanContext->getDevice(), m_indexBuffer, m_indexBufferMemory, 0);
    
    // 复制数据到索引缓冲区
    VkCommandBuffer commandBuffer;
    
    // 简化的命令缓冲区创建（实际项目中应该使用现有的命令池）
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0;
    
    VkCommandPool commandPool;
    vkCreateCommandPool(m_vulkanContext->getDevice(), &poolInfo, nullptr, &commandPool);
    
    VkCommandBufferAllocateInfo allocInfoCmd{};
    allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfoCmd.commandPool = commandPool;
    allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandBufferCount = 1;
    
    vkAllocateCommandBuffers(m_vulkanContext->getDevice(), &allocInfoCmd, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, m_indexBuffer, 1, &copyRegion);
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    // 使用图形队列而不是设备
    vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vulkanContext->getGraphicsQueue());
    
    // 清理临时资源
    vkDestroyBuffer(m_vulkanContext->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_vulkanContext->getDevice(), stagingBufferMemory, nullptr);
    vkDestroyCommandPool(m_vulkanContext->getDevice(), commandPool, nullptr);
    
    Logger::debug("Demo object index buffer created successfully");
}

VkShaderModule DemoObjectRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_vulkanContext->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    
    return shaderModule;
}

uint32_t DemoObjectRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_vulkanContext->getPhysicalDevice(), &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

void DemoObjectRenderer::createShaderModules() {
    Logger::debug("Creating demo object shader modules...");

    m_vertexShaderModule = ShaderCompiler::loadAndCreateModule(
        m_vulkanContext->getDevice(),
        "shaders/demo.vert.spv");

    m_fragmentShaderModule = ShaderCompiler::loadAndCreateModule(
        m_vulkanContext->getDevice(),
        "shaders/demo.frag.spv");

    Logger::debug("Demo object shader modules created successfully");
}

void DemoObjectRenderer::createPipelineLayout() {
    Logger::debug("Creating demo object pipeline layout...");

    // Push constant range for MVP matrix
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_vulkanContext->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    Logger::debug("Demo object pipeline layout created successfully");
}

void DemoObjectRenderer::createGraphicsPipeline() {
    Logger::debug("Creating demo object graphics pipeline...");
    
    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = m_vertexShaderModule;
    vertexShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = m_fragmentShaderModule;
    fragmentShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};
    
    // 顶点输入状态
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(DemoVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    
    // 位置属性
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(DemoVertex, position);
    
    // 颜色属性
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(DemoVertex, color);
    
    // 法线属性
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(DemoVertex, normal);
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪矩形（使用默认值）
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    
    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样状态
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // 深度和模板状态
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // 颜色混合状态
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;
    
    // 动态状态（允许动态修改视口和裁剪矩形）
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    
    // 创建图形管线
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_vulkanContext->getRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    
    if (vkCreateGraphicsPipelines(m_vulkanContext->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
    
    Logger::debug("Demo object graphics pipeline created successfully");
}

void DemoObjectRenderer::createDescriptorSetLayout() {
    // 简化实现，暂时不创建描述符集布局
    Logger::debug("Descriptor set layout creation skipped for simplicity");
}

void DemoObjectRenderer::createDescriptorPool() {
    // 简化实现，暂时不创建描述符池
    Logger::debug("Descriptor pool creation skipped for simplicity");
}

void DemoObjectRenderer::createDescriptorSets() {
    // 简化实现，暂时不创建描述符集
    Logger::debug("Descriptor sets creation skipped for simplicity");
}

void DemoObjectRenderer::draw(VkCommandBuffer commandBuffer) {
    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // Calculate MVP matrix
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix();
    glm::mat4 mvp = proj * view * model;

    // Push MVP matrix
    vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

    // 设置顶点缓冲区
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    // 设置索引缓冲区
    vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // 设置动态视口和裁剪矩形
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_vulkanContext->getSwapchainExtent().width);
    viewport.height = static_cast<float>(m_vulkanContext->getSwapchainExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_vulkanContext->getSwapchainExtent();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // 绘制立方体
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
}

void DemoObjectRenderer::cleanup() {
    Logger::debug("Cleaning up demo object renderer resources...");
    
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_vulkanContext->getDevice(), m_graphicsPipeline, nullptr);
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_vulkanContext->getDevice(), m_pipelineLayout, nullptr);
    }
    
    if (m_fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_vulkanContext->getDevice(), m_fragmentShaderModule, nullptr);
    }
    
    if (m_vertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_vulkanContext->getDevice(), m_vertexShaderModule, nullptr);
    }
    
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_vulkanContext->getDevice(), m_indexBuffer, nullptr);
    }
    
    if (m_indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_vulkanContext->getDevice(), m_indexBufferMemory, nullptr);
    }
    
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_vulkanContext->getDevice(), m_vertexBuffer, nullptr);
    }
    
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_vulkanContext->getDevice(), m_vertexBufferMemory, nullptr);
    }
    
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_vulkanContext->getDevice(), m_descriptorSetLayout, nullptr);
    }
    
    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_vulkanContext->getDevice(), m_descriptorPool, nullptr);
    }
    
    Logger::debug("Demo object renderer resources cleaned up");
}