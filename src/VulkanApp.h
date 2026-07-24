#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <optional>
#include <vector>
#include "PhysicsSystem.h"
#include <sol/sol.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "imgui.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <psapi.h>
#endif

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

enum class ComponentType
{
    TRANSFORM,
    MESH_RENDERER,
    RIGIDBODY_PHYSICS,
    LUA_SCRIPT,
    LIGHT
};

class Component
{
public:
    ComponentType type;
    bool enabled = true;

    virtual ~Component() = default;
    virtual const char* getName() const = 0;
};

class TransformComponent : public Component
{
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    TransformComponent() { type = ComponentType::TRANSFORM; }
    const char* getName() const override { return "📌 Transform"; }
};

class MeshRendererComponent : public Component
{
public:
    int meshId = -1;
    int textureId = -1;
    glm::vec4 color = glm::vec4(1.0f);
    bool visible = true;

    MeshRendererComponent() { type = ComponentType::MESH_RENDERER; }
    const char* getName() const override { return "🧊 Mesh Renderer"; }
};

class RigidBodyComponent : public Component
{
public:
    bool useGravity = true;
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    float linearDrag = 0.01f;
    float angularDrag = 0.05f;
    ColliderType colliderType = ColliderType::BOX;
    BodyMotionType motionType = BodyMotionType::DYNAMIC;
    bool isTrigger = false;
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);

    RigidBodyComponent() { type = ComponentType::RIGIDBODY_PHYSICS; }
    const char* getName() const override { return "⚖️ RigidBody Physics"; }
};

class LuaScriptComponent : public Component
{
public:
    std::string scriptPath = "";
    std::string scriptContent = "";

    LuaScriptComponent() { type = ComponentType::LUA_SCRIPT; }
    const char* getName() const override { return "📖 Lua Script Component"; }
};

class LightComponent : public Component
{
public:
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);
    float intensity = 1.0f;

    LightComponent() { type = ComponentType::LIGHT; }
    const char* getName() const override { return "💡 Light Component"; }
};

struct SceneObject
{
    int id = -1;
    int parentId = -1;
    std::vector<int> children;

    std::string name;
    ObjectType type;

    // Component storage for Hybrid ECS System
    std::vector<std::shared_ptr<Component>> components;

    // Proxy variables for backward compatibility & easy data access
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Physics variables (for play mode)
    glm::vec3 velocity = glm::vec3(0.0f);
    bool isPhysicsEnabled = false;
    std::vector<std::string> luaScripts;
    std::vector<sol::table> luaInstances;
    PhysicsBodyData* bodyData = nullptr;

    // Physics state backup (for Edit→Play→Edit restore)
    glm::vec3 savedPosition = glm::vec3(0.0f);
    glm::vec3 savedRotation = glm::vec3(0.0f);
    glm::vec3 savedScale = glm::vec3(1.0f);

    // Rendering variables
    int meshId = -1;
    int textureId = -1;

    // Component helper methods
    template<typename T>
    std::shared_ptr<T> getComponent() const
    {
        for (const auto& comp : components)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(comp))
                return casted;
        }
        return nullptr;
    }

    bool hasComponent(ComponentType compType) const
    {
        for (const auto& comp : components)
        {
            if (comp->type == compType) return true;
        }
        return false;
    }

    void removeComponent(ComponentType compType)
    {
        components.erase(
            std::remove_if(components.begin(), components.end(),
                [compType](const std::shared_ptr<Component>& c) { return c->type == compType; }),
            components.end());
    }

    void syncComponents()
    {
        if (!hasComponent(ComponentType::TRANSFORM))
        {
            auto trans = std::make_shared<TransformComponent>();
            trans->position = position;
            trans->rotation = rotation;
            trans->scale = scale;
            components.push_back(trans);
        }
        if (meshId >= 0 && !hasComponent(ComponentType::MESH_RENDERER))
        {
            auto mesh = std::make_shared<MeshRendererComponent>();
            mesh->meshId = meshId;
            mesh->textureId = textureId;
            mesh->color = color;
            components.push_back(mesh);
        }
        if (isPhysicsEnabled && !hasComponent(ComponentType::RIGIDBODY_PHYSICS))
        {
            auto rb = std::make_shared<RigidBodyComponent>();
            rb->velocity = velocity;
            components.push_back(rb);
        }
        if (!luaScripts.empty() && !hasComponent(ComponentType::LUA_SCRIPT))
        {
            auto lua = std::make_shared<LuaScriptComponent>();
            if (!luaScripts.empty()) lua->scriptContent = luaScripts[0];
            components.push_back(lua);
        }
        if (type == ObjectType::LIGHT && !hasComponent(ComponentType::LIGHT))
        {
            auto light = std::make_shared<LightComponent>();
            light->color = color;
            components.push_back(light);
        }
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

struct PushConstants {
    glm::mat4 model;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;  // texCoord is vec2
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    uint32_t indexCount;
};

struct Texture {
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
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

class VulkanApp
{
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    glm::mat4 getWorldMatrix(const std::vector<SceneObject>& objects, int index) const;
    void saveHistory();
    void undo();
    void redo();

    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createOffscreenResources();
    
    // Shadow Mapping
    VkRenderPass shadowRenderPass;
    VkImage shadowImage;
    VkDeviceMemory shadowImageMemory;
    VkImageView shadowImageView;
    VkSampler shadowSampler;
    VkFramebuffer shadowFramebuffer;
    VkPipelineLayout shadowPipelineLayout;
    VkPipeline shadowPipeline;
    bool enableShadowMapping = true;

    void createShadowRenderPass();
    void createShadowResources();
    void createShadowPipeline();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void loadModel(const std::string& path, Mesh& mesh);
    int createCubeMesh();
    int createSphereMesh(int stacks = 16, int slices = 32);
    int createPlaneMesh();
    
    // Texture loading helpers
    void loadTexture(const std::string& path, Texture& texture);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createCommandPool();
    void createDepthResources();
    void createFramebuffers();
    void createVertexBuffer();
    void createMeshBuffers(Mesh& mesh);
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createDefaultTexture();
    void createCommandBuffers();
    void createSyncObjects();

    void initImGui();
    void setupUnityStyle();
    void renderImGuiUI();
    ImVec2 projectPoint(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj, const ImVec2& offset, const ImVec2& size);
    bool getRayFromScreenPos(const ImVec2& mousePos, const ImVec2& windowPos, const ImVec2& windowSize, const glm::mat4& view, const glm::mat4& proj, glm::vec3& rayOrigin, glm::vec3& rayDir);
    bool intersectRayPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planePoint, const glm::vec3& planeNormal, glm::vec3& hitPoint);
    float getClosestPointOnAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& pivotPos, const glm::vec3& axisDir, const glm::vec3& cameraPos);
    void drawSceneView(const ImVec2& windowPos, const ImVec2& windowSize);
    void drawGameView(const ImVec2& windowPos, const ImVec2& windowSize);
    void updateProfilerMetrics(float deltaTime);
    void drawProfilerPanel();
    int load3DModelAsset(const std::string& filePath);
    int loadTextureAsset(const std::string& filePath);
    void drawAssetBrowserPanel(float windowWidth, float bottomBarHeight);
    void openFileInExternalEditor(const std::string& filePath);
    void initializeDefaultScene();
    void updatePhysics(float deltaTime);
    void initPhysicsBodies();
    void syncPhysicsToTransform();
    void savePlayModeState();
    void restoreEditModeState();
    void draw3DObject(int objIndex, const glm::mat4& view, const glm::mat4& proj, const ImVec2& offset, const ImVec2& size);
    void saveScene(const std::string& filename);
    void loadScene(const std::string& filename);
    void cleanupImGui();

    void drawFrame();
    void updateUniformBuffer(uint32_t currentImage);
    void recreateSwapChain();
    void cleanupSwapChain();

    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    void updateWindowTitle();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    GLFWwindow* window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout uboSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureSetLayout = VK_NULL_HANDLE;
    // Offscreen Resources (Scene View — uses Scene Camera)
    VkImage offscreenColorImage = VK_NULL_HANDLE;
    VkDeviceMemory offscreenColorImageMemory = VK_NULL_HANDLE;
    VkImageView offscreenColorImageView = VK_NULL_HANDLE;
    VkImage offscreenDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory offscreenDepthImageMemory = VK_NULL_HANDLE;
    VkImageView offscreenDepthImageView = VK_NULL_HANDLE;
    VkRenderPass offscreenRenderPass = VK_NULL_HANDLE;
    VkFramebuffer offscreenFramebuffer = VK_NULL_HANDLE;
    VkSampler offscreenSampler = VK_NULL_HANDLE;
    VkDescriptorSet offscreenDescriptorSet = VK_NULL_HANDLE;

    // Game View offscreen Resources (uses Main Camera)
    VkImage gameColorImage = VK_NULL_HANDLE;
    VkDeviceMemory gameColorImageMemory = VK_NULL_HANDLE;
    VkImageView gameColorImageView = VK_NULL_HANDLE;
    VkImage gameDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory gameDepthImageMemory = VK_NULL_HANDLE;
    VkImageView gameDepthImageView = VK_NULL_HANDLE;
    VkFramebuffer gameFramebuffer = VK_NULL_HANDLE;
    VkSampler gameSampler = VK_NULL_HANDLE;
    VkDescriptorSet gameViewDescriptorSet = VK_NULL_HANDLE;
    // UBOs and descriptor sets for the game camera
    std::vector<VkBuffer> gameUniformBuffers;
    std::vector<VkDeviceMemory> gameUniformBuffersMemory;
    std::vector<void*> gameUniformBuffersMapped;
    std::vector<VkDescriptorSet> gameDescriptorSets;

    // Assets
    std::vector<Mesh> meshes;
    std::vector<Texture> textures;
    VkSampler textureSampler = VK_NULL_HANDLE;
    Texture defaultTexture;  // 1x1 white texture used when object has no texture

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    // Interactive Edit vs Play mode & Unity Engine UI variables
    AppMode mode = AppMode::EDIT;
    bool isDragging = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    float editRotationX = 25.0f; // Pitch angle
    float editRotationY = 45.0f; // Yaw angle
    float editRotationZ = 0.0f;  // Roll angle
    float cameraDistance = 3.464f;
    float autoRotateSpeed = 1.0f;

    // Main Camera Inspector Parameters
    glm::vec3 mainCameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 mainCameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float mainCameraFov = 45.0f;
    float mainCameraNear = 0.1f;
    float mainCameraFar = 20.0f;
    bool showGameViewWindow = true;

    glm::vec3 cubePosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cubeScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec4 backgroundColor = glm::vec4(0.08f, 0.09f, 0.12f, 1.0f);

    std::string selectedGpuName = "Unknown GPU";

    // Dynamic Inspector UI state variables
    SelectedItem selectedItem = SelectedItem::TEXT;
    std::string uiText = "New Text";
    float uiSlider = 0.5f;
    bool uiToggle = true;
    bool uiOptionA = true;
    bool uiOptionB = false;
    bool uiOptionC = false;
    int uiDropdown = 0;
    std::string uiInputField = "Enter text...";
    std::string uiInputArea = "A Scroll Rect is usually used to scroll a large image or panel of another UI element, such as a list of buttons, text, or a large block of text. The Scroll Rect itself is most often used with a mask element and is designed to work seamlessly with scrollbars.";
    int uiButtonClickCount = 0;
    std::string uiScrollViewText = "A Scroll Rect is usually used to scroll a large image or panel of another UI element, such as a list of buttons, text, or a large block of text. The Scroll Rect itself is most often used with a mask element and is designed to work seamlessly with scrollbars.\n\nTo scroll content, the input must be received from inside the bounds of the ScrollRect, not on the content itself.";

    // Scene View navigation parameters (orbiting inside Scene window)
    float sceneRotationX = 25.0f;
    float sceneRotationY = 45.0f;
    float sceneCameraDistance = 5.0f;

    // Game engine scene objects & editor state
    std::vector<SceneObject> sceneObjects;
    std::vector<std::vector<SceneObject>> undoStack;
    std::vector<std::vector<SceneObject>> redoStack;
    int selectedObjectIndex = 0;
    PhysEngine physEngine;
    sol::state luaState;
    bool isDraggingObject = false;
    bool wasDraggingObjectLastFrame = false;
    GizmoType activeGizmo = GizmoType::TRANSLATE;
    DragAxis activeDragAxis = DragAxis::NONE;
    DragAxis hoveredDragAxis = DragAxis::NONE;
    GizmoDragState gizmoDragState;
    ProfilerMetrics profilerMetrics;
    bool showProfilerPanel = true;
    bool showAssetBrowserPanel = true;
    bool isGameFullscreen = false;
    std::unordered_map<std::string, Texture> assetThumbnails;

    // Resizable UI panel dimensions
    float leftPanelWidth = 260.0f;
    float rightPanelWidth = 330.0f;
    float bottomPanelHeight = 220.0f;
    
    // Play mode game state
    int gameScore = 0;
    int highScore = 0;
    
    // Starting coordinates to reset player cube
    glm::vec3 playerStartPos = glm::vec3(0.0f, 0.0f, 0.0f);

    // Cached primitive mesh IDs (created once at startup)
    int primitiveCubeMeshId   = -1;
    int primitiveSphereMeshId = -1;
    int primitivePlaneMeshId  = -1;

    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 720;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};