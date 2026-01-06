#include "CoordinateSystemRenderer.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "Logger.h"
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <array>

// 顶点着色器源码（GLSL）
const std::string vertexShaderSource = R"(
#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";

// 片段着色器源码（GLSL）
const std::string fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
)";

CoordinateSystemRenderer::CoordinateSystemRenderer(VulkanContext* vulkanContext, Camera* camera)
    : m_vulkanContext(vulkanContext), m_camera(camera),
      m_vertexBuffer(VK_NULL_HANDLE), m_vertexBufferMemory(VK_NULL_HANDLE),
      m_indexBuffer(VK_NULL_HANDLE), m_indexBufferMemory(VK_NULL_HANDLE),
      m_vertexShaderModule(VK_NULL_HANDLE), m_fragmentShaderModule(VK_NULL_HANDLE),
      m_pipelineLayout(VK_NULL_HANDLE), m_graphicsPipeline(VK_NULL_HANDLE),
      m_descriptorSetLayout(VK_NULL_HANDLE), m_descriptorPool(VK_NULL_HANDLE),
      m_descriptorSet(VK_NULL_HANDLE),
      m_uniformBuffer(VK_NULL_HANDLE), m_uniformBufferMemory(VK_NULL_HANDLE) {
}

CoordinateSystemRenderer::~CoordinateSystemRenderer() {
    cleanup();
}

bool CoordinateSystemRenderer::init() {
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
        
        Logger::info("CoordinateSystemRenderer initialized successfully!");
        return true;
    } catch (const std::exception& e) {
        Logger::error("CoordinateSystemRenderer initialization error: {}", e.what());
        cleanup();
        return false;
    }
}

void CoordinateSystemRenderer::generateVertexData() {
    Logger::debug("Generating coordinate system vertex data...");
    
    // 清除现有数据
    m_vertices.clear();
    m_indices.clear();
    
    // 坐标轴颜色
    const glm::vec3 xColor(1.0f, 0.0f, 0.0f); // 红色
    const glm::vec3 yColor(0.0f, 1.0f, 0.0f); // 绿色
    const glm::vec3 zColor(0.0f, 0.0f, 1.0f); // 蓝色
    const glm::vec3 gridColor(0.3f, 0.3f, 0.3f); // 灰色
    
    // 生成坐标轴
    uint32_t vertexOffset = 0;
    
    // X轴
    m_vertices.push_back({glm::vec3(0.0f, 0.0f, 0.0f), xColor});
    m_vertices.push_back({glm::vec3(m_axisLength, 0.0f, 0.0f), xColor});
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 1);
    vertexOffset += 2;
    
    // Y轴
    m_vertices.push_back({glm::vec3(0.0f, 0.0f, 0.0f), yColor});
    m_vertices.push_back({glm::vec3(0.0f, m_axisLength, 0.0f), yColor});
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 1);
    vertexOffset += 2;
    
    // Z轴
    m_vertices.push_back({glm::vec3(0.0f, 0.0f, 0.0f), zColor});
    m_vertices.push_back({glm::vec3(0.0f, 0.0f, m_axisLength), zColor});
    m_indices.push_back(vertexOffset);
    m_indices.push_back(vertexOffset + 1);
    vertexOffset += 2;
    
    // 生成刻度线
    float tickLength = m_axisLength / m_tickCount;
    float tickSize = m_axisLength * 0.01f;
    
    // X轴刻度
    for (int i = 1; i <= m_tickCount; i++) {
        float x = i * tickLength;
        
        // 刻度线1 (y方向)
        m_vertices.push_back({glm::vec3(x, -tickSize, 0.0f), xColor});
        m_vertices.push_back({glm::vec3(x, tickSize, 0.0f), xColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
        
        // 刻度线2 (z方向)
        m_vertices.push_back({glm::vec3(x, 0.0f, -tickSize), xColor});
        m_vertices.push_back({glm::vec3(x, 0.0f, tickSize), xColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
    }
    
    // Y轴刻度
    for (int i = 1; i <= m_tickCount; i++) {
        float y = i * tickLength;
        
        // 刻度线1 (x方向)
        m_vertices.push_back({glm::vec3(-tickSize, y, 0.0f), yColor});
        m_vertices.push_back({glm::vec3(tickSize, y, 0.0f), yColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
        
        // 刻度线2 (z方向)
        m_vertices.push_back({glm::vec3(0.0f, y, -tickSize), yColor});
        m_vertices.push_back({glm::vec3(0.0f, y, tickSize), yColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
    }
    
    // Z轴刻度
    for (int i = 1; i <= m_tickCount; i++) {
        float z = i * tickLength;
        
        // 刻度线1 (x方向)
        m_vertices.push_back({glm::vec3(-tickSize, 0.0f, z), zColor});
        m_vertices.push_back({glm::vec3(tickSize, 0.0f, z), zColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
        
        // 刻度线2 (y方向)
        m_vertices.push_back({glm::vec3(0.0f, -tickSize, z), zColor});
        m_vertices.push_back({glm::vec3(0.0f, tickSize, z), zColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
    }
    
    // 生成网格线 (XY平面)
    const int gridSize = 10;
    const float gridSpacing = m_axisLength / gridSize;
    
    for (int i = -gridSize; i <= gridSize; i++) {
        // 垂直线 (x = i * gridSpacing, z = 0)
        float x = i * gridSpacing;
        m_vertices.push_back({glm::vec3(x, -m_axisLength, 0.0f), gridColor});
        m_vertices.push_back({glm::vec3(x, m_axisLength, 0.0f), gridColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
        
        // 水平线 (y = i * gridSpacing, z = 0)
        float y = i * gridSpacing;
        m_vertices.push_back({glm::vec3(-m_axisLength, y, 0.0f), gridColor});
        m_vertices.push_back({glm::vec3(m_axisLength, y, 0.0f), gridColor});
        m_indices.push_back(vertexOffset);
        m_indices.push_back(vertexOffset + 1);
        vertexOffset += 2;
    }
    
    Logger::debug("Generated {} vertices and {} indices for coordinate system", 
                 m_vertices.size(), m_indices.size());
}

// 创建顶点缓冲区
void CoordinateSystemRenderer::createVertexBuffer() {
    Logger::debug("Creating vertex buffer...");
    
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
    
    Logger::debug("Vertex buffer created successfully");
}

// 创建索引缓冲区
void CoordinateSystemRenderer::createIndexBuffer() {
    // 简化实现，暂时不创建索引缓冲区
    Logger::debug("Index buffer creation skipped for simplicity");
}

// 创建着色器模块
VkShaderModule CoordinateSystemRenderer::createShaderModule(const std::vector<char>& code) {
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

// 实现findMemoryType方法
uint32_t CoordinateSystemRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

// 创建着色器模块
void CoordinateSystemRenderer::createShaderModules() {
    Logger::debug("Creating shader modules...");
    
    // 将字符串转换为char向量
    std::vector<char> vertexShaderCode(vertexShaderSource.begin(), vertexShaderSource.end());
    std::vector<char> fragmentShaderCode(fragmentShaderSource.begin(), fragmentShaderSource.end());
    
    m_vertexShaderModule = createShaderModule(vertexShaderCode);
    m_fragmentShaderModule = createShaderModule(fragmentShaderCode);
    
    Logger::debug("Shader modules created successfully");
}

// 创建管线布局
void CoordinateSystemRenderer::createPipelineLayout() {
    Logger::debug("Creating pipeline layout...");
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    
    if (vkCreatePipelineLayout(m_vulkanContext->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    
    Logger::debug("Pipeline layout created successfully");
}

// 创建图形管线
void CoordinateSystemRenderer::createGraphicsPipeline() {
    Logger::debug("Creating graphics pipeline...");
    
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
    bindingDescription.stride = sizeof(CoordinateVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    
    // 位置属性
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(CoordinateVertex, position);
    
    // 颜色属性
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(CoordinateVertex, color);
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样状态
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // 深度和模板状态（禁用）
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
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
    
    Logger::debug("Graphics pipeline created successfully");
}

// 创建描述符集布局
void CoordinateSystemRenderer::createDescriptorSetLayout() {
    // 简化实现，暂时不创建描述符集布局
    Logger::debug("Descriptor set layout creation skipped for simplicity");
}

// 创建描述符池
void CoordinateSystemRenderer::createDescriptorPool() {
    // 简化实现，暂时不创建描述符池
    Logger::debug("Descriptor pool creation skipped for simplicity");
}

// 创建描述符集
void CoordinateSystemRenderer::createDescriptorSets() {
    // 简化实现，暂时不创建描述符集
    Logger::debug("Descriptor sets creation skipped for simplicity");
}

// 绘制坐标系
void CoordinateSystemRenderer::draw(VkCommandBuffer commandBuffer) {
    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    // 设置顶点缓冲区
    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    
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
    
    // 绘制线
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
}

// 清理资源
void CoordinateSystemRenderer::cleanup() {
    Logger::debug("Cleaning up coordinate system renderer resources...");
    
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
    
    Logger::debug("Coordinate system renderer resources cleaned up");
}
