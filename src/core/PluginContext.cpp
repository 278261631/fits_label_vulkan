#include "PluginContext.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "Camera.h"

PluginContext::PluginContext(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera)
    : m_vulkanContext(vulkanContext), m_renderer(renderer), m_camera(camera) {}
