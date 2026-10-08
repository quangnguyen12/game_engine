#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <optional>
#include <vector>
#include <array>
#include <cstdint>
#include <string>

enum class AppMode
{
    PLAY,
    EDIT
};

enum class SelectedItem
{
    CANVAS,
    PANEL,
    TEXT,
    SLIDER,
    TOGGLE,
    TOGGLE_GROUP,
    OPTION_A,
    OPTION_B,
    OPTION_C,
    DROPDOWN,
    INPUT_FIELD,
    INPUT_AREA,
    BUTTON,
    SCROLL_VIEW,
    EVENT_SYSTEM,
    MAIN_CAMERA
};

enum class ObjectType
{
    CUBE,
    SPHERE,
    PLANE,
    LIGHT,
    UI_TEXT,
    UI_BUTTON,
    UI_SLIDER
};

enum class GizmoType
{
    HAND,
    TRANSLATE,
    ROTATE,
    SCALE,
    RECT,
    TRANSFORM_COMBINED
};

enum class DragAxis
{
    NONE,
    X,
    Y,
    Z,
    XY,
    YZ,
    XZ,
    FREE
};

struct GizmoDragState
{
    bool isDragging = false;
    DragAxis axis = DragAxis::NONE;
    GizmoType gizmoType = GizmoType::TRANSLATE;

    glm::vec3 startObjPos = glm::vec3(0.0f);
    glm::vec3 startObjRot = glm::vec3(0.0f);
    glm::vec3 startObjScale = glm::vec3(1.0f);
    glm::vec3 pivotPos = glm::vec3(0.0f);

    glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 planePoint = glm::vec3(0.0f);
    glm::vec3 axisDir = glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 startHitPoint = glm::vec3(0.0f);
    float startAxisVal = 0.0f;
    float startDist = 1.0f;
    float startAngle = 0.0f;

    glm::vec3 rotBasisU = glm::vec3(0.0f);
    glm::vec3 rotBasisV = glm::vec3(0.0f);
};

struct ProfilerMetrics
{
    float frameTimeMs = 0.0f;
    float fps = 0.0f;
    float cpuUsagePercent = 0.0f;
    float ramUsageMB = 0.0f;
    float vramUsageMB = 0.0f;

    std::vector<float> frameTimeHistory;
    std::vector<float> cpuHistory;
    std::vector<float> ramHistory;
    std::vector<float> vramHistory;

    float minFrameTime = 999.0f;
    float maxFrameTime = 0.0f;
    float avgFrameTime = 0.0f;

    ProfilerMetrics()
    {
        frameTimeHistory.resize(60, 0.0f);
        cpuHistory.resize(60, 0.0f);
        ramHistory.resize(60, 0.0f);
        vramHistory.resize(60, 0.0f);
    }
};

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct PushConstants
{
    glm::mat4 model;
    alignas(16) glm::vec4 pbrParams;     // x: roughness, y: metallic, z: usePBR (1.0 or 0.0), w: ao
    alignas(16) glm::vec4 foliageParams; // x: alphaCutoff (0.0=0.35), y: isFoliage (1.0), z: twoSided (1.0), w: sssIntensity
};

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 lightColor;
    alignas(16) glm::vec3 viewPos;
    alignas(16) glm::mat4 lightSpaceMatrix;
    alignas(16) glm::vec3 lightDir;
    alignas(16) float enableShadows;
};
