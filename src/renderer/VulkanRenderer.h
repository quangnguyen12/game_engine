#pragma once

#include "core/Types.h"
#include "core/Vertex.h"
#include <functional>
#include <string>
#include <vector>

class Scene;
class AssetManager;

struct CameraData
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
    glm::vec3 viewPos = glm::vec3(0.0f);
};

class VulkanRenderer
{
public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 720;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    VulkanRenderer();
    ~VulkanRenderer();

    void init(GLFWwindow* window);
    void cleanup();

    void recreateSwapChain();
    void cleanupSwapChain();

    void updateUniformBuffer(uint32_t currentImage, const Scene& scene, const CameraData& sceneCam, const CameraData& gameCam);
    void drawFrame(const Scene& scene, const AssetManager& assetManager, std::function<void()> onRecordUi);

    // Vulkan helper methods
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkDescriptorSet createTextureDescriptorSet(VkImageView imageView, VkSampler sampler);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    // Accessors
    VkDevice getDevice() const { return device; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
    VkInstance getInstance() const { return instance; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkRenderPass getRenderPass() const { return renderPass; }
    VkDescriptorPool getDescriptorPool() const { return descriptorPool; }
    VkDescriptorPool getImguiDescriptorPool() const { return imguiDescriptorPool; }
    VkDescriptorSetLayout getTextureSetLayout() const { return textureSetLayout; }
    VkDescriptorSet getOffscreenDescriptorSet() const { return offscreenDescriptorSet; }
    VkDescriptorSet getGameViewDescriptorSet() const { return gameViewDescriptorSet; }
    const std::string& getGpuName() const { return selectedGpuName; }
    VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
    uint32_t getCurrentFrame() const { return currentFrame; }

    VkSampler getOffscreenSampler() const { return offscreenSampler; }
    VkImageView getOffscreenColorImageView() const { return offscreenColorImageView; }
    VkSampler getGameSampler() const { return gameSampler; }
    VkImageView getGameColorImageView() const { return gameColorImageView; }
    const std::vector<VkImage>& getSwapChainImages() const { return swapChainImages; }
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice dev);

    void setOffscreenDescriptorSet(VkDescriptorSet set) { offscreenDescriptorSet = set; }
    void setGameViewDescriptorSet(VkDescriptorSet set) { gameViewDescriptorSet = set; }
    void setFramebufferResized(bool resized) { framebufferResized = resized; }
    bool isShadowMappingEnabled() const { return enableShadowMapping; }
    void setShadowMappingEnabled(bool enabled) { enableShadowMapping = enabled; }

    glm::vec4& getBackgroundColor() { return backgroundColor; }
    const glm::vec4& getBackgroundColor() const { return backgroundColor; }

private:
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createCommandPool();
    void createDepthResources();
    void createFramebuffers();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    void createOffscreenResources();
    void createShadowRenderPass();
    void createShadowResources();
    void createShadowPipeline();

    bool isDeviceSuitable(VkPhysicalDevice dev);
    bool checkDeviceExtensionSupport(VkPhysicalDevice dev);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice dev);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

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

    // Offscreen Resources (Scene View)
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

    // Game View offscreen Resources
    VkImage gameColorImage = VK_NULL_HANDLE;
    VkDeviceMemory gameColorImageMemory = VK_NULL_HANDLE;
    VkImageView gameColorImageView = VK_NULL_HANDLE;
    VkImage gameDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory gameDepthImageMemory = VK_NULL_HANDLE;
    VkImageView gameDepthImageView = VK_NULL_HANDLE;
    VkFramebuffer gameFramebuffer = VK_NULL_HANDLE;
    VkSampler gameSampler = VK_NULL_HANDLE;
    VkDescriptorSet gameViewDescriptorSet = VK_NULL_HANDLE;
    std::vector<VkBuffer> gameUniformBuffers;
    std::vector<VkDeviceMemory> gameUniformBuffersMemory;
    std::vector<void*> gameUniformBuffersMapped;
    std::vector<VkDescriptorSet> gameDescriptorSets;

    // Shadow Mapping
    VkRenderPass shadowRenderPass = VK_NULL_HANDLE;
    VkImage shadowImage = VK_NULL_HANDLE;
    VkDeviceMemory shadowImageMemory = VK_NULL_HANDLE;
    VkImageView shadowImageView = VK_NULL_HANDLE;
    VkSampler shadowSampler = VK_NULL_HANDLE;
    VkFramebuffer shadowFramebuffer = VK_NULL_HANDLE;
    VkPipelineLayout shadowPipelineLayout = VK_NULL_HANDLE;
    VkPipeline shadowPipeline = VK_NULL_HANDLE;
    bool enableShadowMapping = true;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

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

    glm::vec4 backgroundColor = glm::vec4(0.08f, 0.09f, 0.12f, 1.0f);
    std::string selectedGpuName = "Unknown GPU";
};
