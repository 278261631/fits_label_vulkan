#pragma once

#include <vector>
#include <functional>

class VulkanContext;
class Renderer;
class Camera;

// Point data structure for plugins to submit
struct PluginPointData {
    float x, y, z;
    float r, g, b;
    float size;  // point size
};

class PluginContext {
public:
    PluginContext(VulkanContext* vulkanContext, Renderer* renderer, Camera* camera)
        : m_vulkanContext(vulkanContext), m_renderer(renderer), m_camera(camera),
          m_pointCloudDirty(false) {}

    VulkanContext* getVulkanContext() const { return m_vulkanContext; }
    Renderer* getRenderer() const { return m_renderer; }
    Camera* getCamera() const { return m_camera; }

    // Point cloud interface for plugins - inline implementation
    void setPointCloudData(const std::vector<PluginPointData>& points) {
        m_pointCloudData = points;
        m_pointCloudDirty = true;
    }

    const std::vector<PluginPointData>& getPointCloudData() const { return m_pointCloudData; }
    bool hasPointCloudData() const { return !m_pointCloudData.empty(); }
    void clearPointCloudData() { m_pointCloudData.clear(); m_pointCloudDirty = true; }

    bool isPointCloudDirty() const { return m_pointCloudDirty; }
    void setPointCloudDirty(bool dirty) { m_pointCloudDirty = dirty; }

private:
    VulkanContext* m_vulkanContext;
    Renderer* m_renderer;
    Camera* m_camera;

    std::vector<PluginPointData> m_pointCloudData;
    bool m_pointCloudDirty = false;
};
