#pragma once

#include "core/Types.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class ProfilerPanel
{
public:
    ProfilerPanel();

    void update(float deltaTime, VkPhysicalDevice physicalDevice);
    void draw();

    bool& getVisible() { return showProfilerPanel; }
    const ProfilerMetrics& getMetrics() const { return profilerMetrics; }

private:
    ProfilerMetrics profilerMetrics;
    bool showProfilerPanel = true;
};
