#include "GridRenderer.h"
#include "VulkanContext.h"
#include "Camera.h"
#include "Logger.h"
#include "ShaderCompiler.h"
#include <imgui.h>
#include <stdexcept>
#include <array>
#include <sstream>
#include <iomanip>
#include <glm/gtc/matrix_transform.hpp>

GridRenderer::GridRenderer(VulkanContext* vulkanContext, Camera* camera)
    : m_vulkanContext(vulkanContext), m_camera(camera),
      m_vertexBuffer(VK_NULL_HANDLE), m_vertexBufferMemory(VK_NULL_HANDLE),
      m_vertexShaderModule(VK_NULL_HANDLE), m_fragmentShaderModule(VK_NULL_HANDLE),
      m_pipelineLayout(VK_NULL_HANDLE), m_graphicsPipeline(VK_NULL_HANDLE) {
}

GridRenderer::~GridRenderer() {
    cleanup();
}

bool GridRenderer::init() {
    try {
        generateVertexData();
        generateLabels();
        createVertexBuffer();
        createShaderModules();
        createPipelineLayout();
        createGraphicsPipeline();

        Logger::info("GridRenderer initialized successfully!");
        return true;
    } catch (const std::exception& e) {
        Logger::error("GridRenderer initialization error: {}", e.what());
        cleanup();
        return false;
    }
}

void GridRenderer::generateVertexData() {
    Logger::debug("Generating grid vertex data...");
    
    m_vertices.clear();
    
    // Colors
    const glm::vec3 majorGridColor(0.4f, 0.4f, 0.4f);
    const glm::vec3 minorGridColor(0.25f, 0.25f, 0.25f);
    const glm::vec3 xAxisColor(0.8f, 0.2f, 0.2f);
    const glm::vec3 yAxisColor(0.2f, 0.8f, 0.2f);
    const glm::vec3 zAxisColor(0.2f, 0.2f, 0.8f);
    
    int gridCount = static_cast<int>(m_gridSize / m_gridSpacing);
    
    // XZ plane grid (ground plane)
    for (int i = -gridCount; i <= gridCount; i++) {
        float pos = i * m_gridSpacing;
        glm::vec3 color = (i % 5 == 0) ? majorGridColor : minorGridColor;
        
        // Lines parallel to X axis
        m_vertices.push_back({glm::vec3(-m_gridSize, 0.0f, pos), color});
        m_vertices.push_back({glm::vec3(m_gridSize, 0.0f, pos), color});
        
        // Lines parallel to Z axis
        m_vertices.push_back({glm::vec3(pos, 0.0f, -m_gridSize), color});
        m_vertices.push_back({glm::vec3(pos, 0.0f, m_gridSize), color});
    }
    
    // XY plane grid (back plane)
    for (int i = -gridCount; i <= gridCount; i++) {
        float pos = i * m_gridSpacing;
        glm::vec3 color = (i % 5 == 0) ? majorGridColor : minorGridColor;
        color *= 0.5f; // Dimmer for back plane
        
        // Horizontal lines
        m_vertices.push_back({glm::vec3(-m_gridSize, pos, -m_gridSize), color});
        m_vertices.push_back({glm::vec3(m_gridSize, pos, -m_gridSize), color});
        
        // Vertical lines
        m_vertices.push_back({glm::vec3(pos, -m_gridSize, -m_gridSize), color});
        m_vertices.push_back({glm::vec3(pos, m_gridSize, -m_gridSize), color});
    }
    
    // YZ plane grid (side plane)
    for (int i = -gridCount; i <= gridCount; i++) {
        float pos = i * m_gridSpacing;
        glm::vec3 color = (i % 5 == 0) ? majorGridColor : minorGridColor;
        color *= 0.5f;
        
        // Horizontal lines (along Z)
        m_vertices.push_back({glm::vec3(-m_gridSize, pos, -m_gridSize), color});
        m_vertices.push_back({glm::vec3(-m_gridSize, pos, m_gridSize), color});
        
        // Vertical lines (along Y)
        m_vertices.push_back({glm::vec3(-m_gridSize, -m_gridSize, pos), color});
        m_vertices.push_back({glm::vec3(-m_gridSize, m_gridSize, pos), color});
    }
    
    // Main axes (thicker, brighter)
    // X axis (red)
    m_vertices.push_back({glm::vec3(-m_gridSize, 0.0f, 0.0f), xAxisColor});
    m_vertices.push_back({glm::vec3(m_gridSize, 0.0f, 0.0f), xAxisColor});
    
    // Y axis (green)
    m_vertices.push_back({glm::vec3(0.0f, -m_gridSize, 0.0f), yAxisColor});
    m_vertices.push_back({glm::vec3(0.0f, m_gridSize, 0.0f), yAxisColor});
    
    // Z axis (blue)
    m_vertices.push_back({glm::vec3(0.0f, 0.0f, -m_gridSize), zAxisColor});
    m_vertices.push_back({glm::vec3(0.0f, 0.0f, m_gridSize), zAxisColor});
    
    // Tick marks on axes
    float tickSize = m_gridSpacing * 0.1f;
    for (int i = -gridCount; i <= gridCount; i++) {
        if (i == 0) continue;
        float pos = i * m_gridSpacing;
        
        // X axis ticks
        m_vertices.push_back({glm::vec3(pos, -tickSize, 0.0f), xAxisColor});
        m_vertices.push_back({glm::vec3(pos, tickSize, 0.0f), xAxisColor});
        
        // Y axis ticks
        m_vertices.push_back({glm::vec3(-tickSize, pos, 0.0f), yAxisColor});
        m_vertices.push_back({glm::vec3(tickSize, pos, 0.0f), yAxisColor});
        
        // Z axis ticks
        m_vertices.push_back({glm::vec3(0.0f, -tickSize, pos), zAxisColor});
        m_vertices.push_back({glm::vec3(0.0f, tickSize, pos), zAxisColor});
    }
    
    Logger::debug("Generated {} vertices for grid", m_vertices.size());
}

void GridRenderer::createVertexBuffer() {
    Logger::debug("Creating grid vertex buffer...");

    VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

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
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_vulkanContext->getDevice(), &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(m_vulkanContext->getDevice(), stagingBuffer, stagingBufferMemory, 0);

    void* data;
    vkMapMemory(m_vulkanContext->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(m_vulkanContext->getDevice(), stagingBufferMemory);

    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (vkCreateBuffer(m_vulkanContext->getDevice(), &bufferInfo, nullptr, &m_vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex buffer!");
    }

    vkGetBufferMemoryRequirements(m_vulkanContext->getDevice(), m_vertexBuffer, &memRequirements);
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_vulkanContext->getDevice(), &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(m_vulkanContext->getDevice(), m_vertexBuffer, m_vertexBufferMemory, 0);

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

    VkCommandBuffer commandBuffer;
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

    vkQueueSubmit(m_vulkanContext->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vulkanContext->getGraphicsQueue());

    vkDestroyBuffer(m_vulkanContext->getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_vulkanContext->getDevice(), stagingBufferMemory, nullptr);
    vkDestroyCommandPool(m_vulkanContext->getDevice(), commandPool, nullptr);

    Logger::debug("Grid vertex buffer created successfully");
}

void GridRenderer::generateLabels() {
    m_labels.clear();

    int halfGrid = static_cast<int>(m_gridSize / m_gridSpacing);
    Logger::info("GridRenderer: generating labels, halfGrid={}, gridSize={}, gridSpacing={}",
                 halfGrid, m_gridSize, m_gridSpacing);

    // X axis labels (red)
    for (int i = -halfGrid; i <= halfGrid; i++) {
        if (i == 0) continue;  // Skip origin
        float x = i * m_gridSpacing;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << x;
        m_labels.push_back(GridLabel(
            glm::vec3(x, -0.3f, 0.0f),
            oss.str(),
            glm::vec3(1.0f, 0.3f, 0.3f)
        ));
    }

    // Y axis labels (green)
    for (int i = -halfGrid; i <= halfGrid; i++) {
        if (i == 0) continue;
        float y = i * m_gridSpacing;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << y;
        m_labels.push_back(GridLabel(
            glm::vec3(-0.3f, y, 0.0f),
            oss.str(),
            glm::vec3(0.3f, 1.0f, 0.3f)
        ));
    }

    // Z axis labels (blue)
    for (int i = -halfGrid; i <= halfGrid; i++) {
        if (i == 0) continue;
        float z = i * m_gridSpacing;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << z;
        m_labels.push_back(GridLabel(
            glm::vec3(0.0f, -0.3f, z),
            oss.str(),
            glm::vec3(0.3f, 0.3f, 1.0f)
        ));
    }

    // Origin label
    m_labels.push_back(GridLabel(
        glm::vec3(-0.3f, -0.3f, 0.0f),
        "0",
        glm::vec3(1.0f, 1.0f, 1.0f)
    ));

    // Axis name labels
    m_labels.push_back(GridLabel(
        glm::vec3(m_gridSize + 0.5f, 0.0f, 0.0f),
        "X",
        glm::vec3(1.0f, 0.2f, 0.2f)
    ));
    m_labels.push_back(GridLabel(
        glm::vec3(0.0f, m_gridSize + 0.5f, 0.0f),
        "Y",
        glm::vec3(0.2f, 1.0f, 0.2f)
    ));
    m_labels.push_back(GridLabel(
        glm::vec3(0.0f, 0.0f, m_gridSize + 0.5f),
        "Z",
        glm::vec3(0.2f, 0.2f, 1.0f)
    ));

    Logger::info("GridRenderer: generated {} labels", m_labels.size());
}

glm::vec2 GridRenderer::worldToScreen(const glm::vec3& worldPos) {
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix();
    glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);

    if (clipPos.w <= 0.0f) {
        return glm::vec2(-10000.0f, -10000.0f);  // Behind camera
    }

    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;

    VkExtent2D extent = m_vulkanContext->getSwapchainExtent();
    float screenX = (ndc.x + 1.0f) * 0.5f * extent.width;
    float screenY = (1.0f - ndc.y) * 0.5f * extent.height;  // Flip Y for screen coords

    return glm::vec2(screenX, screenY);
}

void GridRenderer::drawLabels() {
    if (!m_showLabels) return;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();  // Use foreground so labels appear on top
    if (!drawList) {
        Logger::warn("GridRenderer::drawLabels - drawList is null!");
        return;
    }

    int drawnCount = 0;
    for (const auto& label : m_labels) {
        glm::vec2 screenPos = worldToScreen(label.worldPos);

        // Skip if behind camera or off screen
        if (screenPos.x < -100 || screenPos.y < -100) continue;

        VkExtent2D extent = m_vulkanContext->getSwapchainExtent();
        if (screenPos.x > extent.width + 100 || screenPos.y > extent.height + 100) continue;

        ImU32 color = IM_COL32(
            static_cast<int>(label.color.r * 255),
            static_cast<int>(label.color.g * 255),
            static_cast<int>(label.color.b * 255),
            255
        );

        // Draw text with slight shadow for visibility
        drawList->AddText(
            ImVec2(screenPos.x + 1, screenPos.y + 1),
            IM_COL32(0, 0, 0, 180),
            label.text.c_str()
        );
        drawList->AddText(
            ImVec2(screenPos.x, screenPos.y),
            color,
            label.text.c_str()
        );
        drawnCount++;
    }

    // Log once every 100 frames
    static int frameCounter = 0;
    if (frameCounter++ % 100 == 0) {
        Logger::info("GridRenderer::drawLabels - drew {} labels out of {}", drawnCount, m_labels.size());
    }
}

uint32_t GridRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

void GridRenderer::createShaderModules() {
    Logger::debug("Creating grid shader modules...");

    m_vertexShaderModule = ShaderCompiler::loadAndCreateModule(
        m_vulkanContext->getDevice(),
        "shaders/grid.vert.spv");

    m_fragmentShaderModule = ShaderCompiler::loadAndCreateModule(
        m_vulkanContext->getDevice(),
        "shaders/grid.frag.spv");

    Logger::debug("Grid shader modules created successfully");
}

void GridRenderer::createPipelineLayout() {
    Logger::debug("Creating grid pipeline layout...");

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

    Logger::debug("Grid pipeline layout created successfully");
}

void GridRenderer::createGraphicsPipeline() {
    Logger::debug("Creating grid graphics pipeline...");

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

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(GridVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(GridVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(GridVertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

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

    if (vkCreateGraphicsPipelines(m_vulkanContext->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    Logger::debug("Grid graphics pipeline created successfully");
}

void GridRenderer::rebuildIfNeeded() {
    if (!m_needsRebuild) return;

    vkDeviceWaitIdle(m_vulkanContext->getDevice());

    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_vulkanContext->getDevice(), m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_vulkanContext->getDevice(), m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    generateVertexData();
    generateLabels();
    createVertexBuffer();

    m_needsRebuild = false;
}

void GridRenderer::draw(VkCommandBuffer commandBuffer) {
    rebuildIfNeeded();

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // Calculate MVP matrix
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = m_camera->getViewMatrix();
    glm::mat4 proj = m_camera->getProjectionMatrix();
    glm::mat4 mvp = proj * view * model;

    // Push MVP matrix
    vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

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

    vkCmdDraw(commandBuffer, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
}

void GridRenderer::cleanup() {
    Logger::debug("Cleaning up grid renderer resources...");

    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_vulkanContext->getDevice(), m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_vulkanContext->getDevice(), m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_fragmentShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_vulkanContext->getDevice(), m_fragmentShaderModule, nullptr);
        m_fragmentShaderModule = VK_NULL_HANDLE;
    }

    if (m_vertexShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(m_vulkanContext->getDevice(), m_vertexShaderModule, nullptr);
        m_vertexShaderModule = VK_NULL_HANDLE;
    }

    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_vulkanContext->getDevice(), m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_vulkanContext->getDevice(), m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    Logger::debug("Grid renderer resources cleaned up");
}