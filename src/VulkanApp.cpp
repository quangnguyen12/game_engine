#include "VulkanApp.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_DONT_INCLUDE_FAST_FLOAT
#include "tiny_obj_loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "tinyfiledialogs.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <set>
#include <cstring>
#include <sstream>
#include <filesystem>

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const std::vector<Vertex> vertices = {
    // Front face (Vibrant Red/Orange)
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.2f, 0.3f}},
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.5f, 0.2f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.8f, 0.2f}},
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.3f, 0.5f}},

    // Back face (Vibrant Green/Emerald)
    {{ 0.5f, -0.5f, -0.5f}, {0.1f, 0.9f, 0.3f}},
    {{-0.5f, -0.5f, -0.5f}, {0.2f, 1.0f, 0.5f}},
    {{-0.5f,  0.5f, -0.5f}, {0.3f, 0.9f, 0.7f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.1f, 0.8f, 0.4f}},

    // Top face (Vibrant Blue/Cyan)
    {{-0.5f, -0.5f, -0.5f}, {0.1f, 0.5f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.2f, 0.6f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.4f, 0.2f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.1f, 0.4f, 1.0f}},

    // Bottom face (Vibrant Yellow/Gold)
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.9f, 0.1f}},
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.7f, 0.2f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.9f, 0.3f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.8f, 0.1f}},

    // Right face (Vibrant Purple/Pink)
    {{ 0.5f, -0.5f,  0.5f}, {0.9f, 0.2f, 0.9f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.7f, 0.1f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.3f, 0.9f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.8f, 0.2f, 1.0f}},

    // Left face (Vibrant Teal/Cyan)
    {{-0.5f, -0.5f, -0.5f}, {0.1f, 0.9f, 0.9f}},
    {{-0.5f, -0.5f,  0.5f}, {0.2f, 1.0f, 0.8f}},
    {{-0.5f,  0.5f,  0.5f}, {0.1f, 0.8f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.3f, 0.9f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0,  1,  2,  2,  3,  0,  // Front
    4,  5,  6,  6,  7,  4,  // Back
    8,  9,  10, 10, 11, 8,  // Top
    12, 13, 14, 14, 15, 12, // Bottom
    16, 17, 18, 18, 19, 16, // Right
    20, 21, 22, 22, 23, 20  // Left
};

static std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

static const char* getDeviceTypeString(VkPhysicalDeviceType type)
{
    switch (type)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "Discrete GPU (GPU rời)";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "Integrated GPU (GPU tích hợp)";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "Virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "CPU (Software Renderer)";
    default:
        return "Unknown GPU";
    }
}

static int rateDeviceSuitability(VkPhysicalDevice device, const VkPhysicalDeviceProperties& properties)
{
    int score = 0;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 10000;
    }
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 100;
    }

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryHeapCount; i++)
    {
        if (memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            score += static_cast<int>(memProperties.memoryHeaps[i].size / (1024 * 1024));
        }
    }

    return score;
}

void VulkanApp::run()
{
    initWindow();
    initVulkan();
    physEngine.init();

    // Initialize Lua & Sol2
    luaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);
    
    // Bind SceneObject to Lua
    luaState.new_usertype<SceneObject>("SceneObject",
        "name", &SceneObject::name,
        "position", &SceneObject::position,
        "rotation", &SceneObject::rotation,
        "scale", &SceneObject::scale,
        "velocity", &SceneObject::velocity
    );
    
    // Bind glm::vec3
    luaState.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

    // Bind Input API to Lua
    auto inputTable = luaState.create_named_table("Input");
    inputTable.set_function("isKeyPressed", [this](const std::string& key) -> bool {
        if (!window) return false;
        std::string k = key;
        for (auto& c : k) c = static_cast<char>(toupper(c));

        int glfwKey = -1;
        if (k == "W") glfwKey = GLFW_KEY_W;
        else if (k == "A") glfwKey = GLFW_KEY_A;
        else if (k == "S") glfwKey = GLFW_KEY_S;
        else if (k == "D") glfwKey = GLFW_KEY_D;
        else if (k == "UP") glfwKey = GLFW_KEY_UP;
        else if (k == "DOWN") glfwKey = GLFW_KEY_DOWN;
        else if (k == "LEFT") glfwKey = GLFW_KEY_LEFT;
        else if (k == "RIGHT") glfwKey = GLFW_KEY_RIGHT;
        else if (k == "SPACE") glfwKey = GLFW_KEY_SPACE;
        else if (k == "SHIFT") glfwKey = GLFW_KEY_LEFT_SHIFT;
        else if (k == "CTRL") glfwKey = GLFW_KEY_LEFT_CONTROL;
        else if (k.length() == 1 && k[0] >= 'A' && k[0] <= 'Z') glfwKey = GLFW_KEY_A + (k[0] - 'A');
        else if (k.length() == 1 && k[0] >= '0' && k[0] <= '9') glfwKey = GLFW_KEY_0 + (k[0] - '0');

        if (glfwKey != -1) {
            return glfwGetKey(window, glfwKey) == GLFW_PRESS;
        }
        return false;
    });
    // Create shared primitive meshes once (requires Vulkan device to be ready)
    primitiveCubeMeshId   = createCubeMesh();
    primitiveSphereMeshId = createSphereMesh();
    primitivePlaneMeshId  = createPlaneMesh();
    initializeDefaultScene();
    initImGui();
    mainLoop();
    cleanup();
}

void VulkanApp::initWindow()
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Unity Hub 3D Vulkan Engine", nullptr, nullptr);
    if (!window)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);

    updateWindowTitle();
}

void VulkanApp::updateWindowTitle()
{
    if (mode == AppMode::PLAY)
    {
        glfwSetWindowTitle(window, "Unity Hub 3D Vulkan Engine | [PLAY MODE]");
    }
    else
    {
        glfwSetWindowTitle(window, "Unity Hub 3D Vulkan Engine | [EDIT MODE]");
    }
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_TAB || key == GLFW_KEY_E)
        {
            if (app->mode == AppMode::PLAY)
            {
                app->mode = AppMode::EDIT;
            }
            else
            {
                app->mode = AppMode::PLAY;
            }
            app->updateWindowTitle();
        }
    }
}

void VulkanApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            app->isDragging = true;
            glfwGetCursorPos(window, &app->lastMouseX, &app->lastMouseY);
        }
        else if (action == GLFW_RELEASE)
        {
            app->isDragging = false;
        }
    }
}

void VulkanApp::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

    if (app->isDragging && app->mode == AppMode::EDIT)
    {
        double dx = xpos - app->lastMouseX;
        double dy = ypos - app->lastMouseY;

        app->editRotationY += static_cast<float>(dx) * 0.5f;
        app->editRotationX += static_cast<float>(dy) * 0.5f;

        app->lastMouseX = xpos;
        app->lastMouseY = ypos;
    }
}

void VulkanApp::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

    if (app->mode == AppMode::EDIT)
    {
        app->cameraDistance -= static_cast<float>(yoffset) * 0.3f;
        app->cameraDistance = std::clamp(app->cameraDistance, 1.0f, 15.0f);
    }
}

void VulkanApp::createOffscreenResources()
{
    // Color attachment
    createImage(WIDTH, HEIGHT, swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenColorImage, offscreenColorImageMemory);
    offscreenColorImageView = createImageView(offscreenColorImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    // Depth attachment
    VkFormat depthFormat = findDepthFormat();
    createImage(WIDTH, HEIGHT, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenDepthImage, offscreenDepthImageMemory);
    offscreenDepthImageView = createImageView(offscreenDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Render Pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // For ImGui

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Dependency 1: External → Subpass 0 (prepare for rendering)
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // Dependency 2: Subpass 0 → External (transition to shader-readable for ImGui)
    VkSubpassDependency dependency2{};
    dependency2.srcSubpass = 0;
    dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkSubpassDependency, 2> dependencies = {dependency, dependency2};
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenRenderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create offscreen render pass!");

    // Framebuffer
    std::array<VkImageView, 2> fbAttachments = {offscreenColorImageView, offscreenDepthImageView};
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = offscreenRenderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
    framebufferInfo.pAttachments = fbAttachments.data();
    framebufferInfo.width = WIDTH;
    framebufferInfo.height = HEIGHT;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &offscreenFramebuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create offscreen framebuffer!");

    // Sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &offscreenSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create offscreen sampler!");

    // ---- Game View Framebuffer (uses Main Camera, reuses offscreenRenderPass) ----
    VkFormat gameDepthFormat = findDepthFormat();
    createImage(WIDTH, HEIGHT, swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gameColorImage, gameColorImageMemory);
    gameColorImageView = createImageView(gameColorImage, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    createImage(WIDTH, HEIGHT, gameDepthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gameDepthImage, gameDepthImageMemory);
    gameDepthImageView = createImageView(gameDepthImage, gameDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    std::array<VkImageView, 2> gameAttachments = {gameColorImageView, gameDepthImageView};
    VkFramebufferCreateInfo gameFbInfo{};
    gameFbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    gameFbInfo.renderPass = offscreenRenderPass; // reuse same render pass
    gameFbInfo.attachmentCount = static_cast<uint32_t>(gameAttachments.size());
    gameFbInfo.pAttachments = gameAttachments.data();
    gameFbInfo.width = WIDTH;
    gameFbInfo.height = HEIGHT;
    gameFbInfo.layers = 1;
    if (vkCreateFramebuffer(device, &gameFbInfo, nullptr, &gameFramebuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create game framebuffer!");

    // Sampler for game view (same settings)
    if (vkCreateSampler(device, &samplerInfo, nullptr, &gameSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create game sampler!");
}


VkCommandBuffer VulkanApp::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void VulkanApp::loadTexture(const std::string& path, Texture& texture) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    stbi_image_free(pixels);
    
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.image, texture.memory);
    
    transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    texture.view = createImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    
    // Create descriptor set for this texture
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &texture.descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate texture descriptor set!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture.view;
    imageInfo.sampler = offscreenSampler; // Reuse the sampler we created earlier

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = texture.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

void VulkanApp::createDefaultTexture() {
    VkDeviceSize imageSize = 4;
    uint8_t pixels[4] = {255, 255, 255, 255};
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    
    createImage(1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, defaultTexture.image, defaultTexture.memory);
    
    transitionImageLayout(defaultTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, defaultTexture.image, 1, 1);
    transitionImageLayout(defaultTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    defaultTexture.view = createImageView(defaultTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &textureSetLayout;

    if (vkAllocateDescriptorSets(device, &allocInfo, &defaultTexture.descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate default texture descriptor set!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = defaultTexture.view;
    imageInfo.sampler = offscreenSampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = defaultTexture.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
}

void VulkanApp::createMeshBuffers(Mesh& mesh) {
    // Vertex Buffer
    VkDeviceSize bufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, mesh.vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertexBuffer, mesh.vertexBufferMemory);
    
    copyBuffer(stagingBuffer, mesh.vertexBuffer, bufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    
    // Index Buffer
    bufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, mesh.indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);
    
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer, mesh.indexBufferMemory);
    
    copyBuffer(stagingBuffer, mesh.indexBuffer, bufferSize);
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanApp::loadModel(const std::string& path, Mesh& mesh)
{
    mesh.vertices.clear();
    mesh.indices.clear();

    std::string ext = path.substr(path.find_last_of('.') + 1);
    for(auto& c : ext) c = tolower(c);

    if (ext == "glb" || ext == "gltf") {
        tinygltf::Model gltfModel;
        tinygltf::TinyGLTF loader;
        std::string err, warn;
        bool ret = false;
        
        // Dummy image loader to prevent "No LoadImageData callback specified" error 
        // since we defined TINYGLTF_NO_STB_IMAGE and don't need textures from GLTF yet.
        loader.SetImageLoader([](tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*) -> bool {
            return true;
        }, nullptr);
        
        if (ext == "glb") {
            ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);
        } else {
            ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path);
        }
        
        if (!warn.empty()) printf("GLTF Warn: %s\n", warn.c_str());
        if (!err.empty()) printf("GLTF Err: %s\n", err.c_str());
        if (!ret) throw std::runtime_error("Failed to parse glTF: " + path);
        
        const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
        
        // Simple recursive node parsing to extract all meshes
        std::function<void(int, const glm::mat4&)> processNode = [&](int nodeIndex, const glm::mat4& parentMatrix) {
            const tinygltf::Node& node = gltfModel.nodes[nodeIndex];
            glm::mat4 matrix = parentMatrix;
            
            // local transform
            if (node.matrix.size() == 16) {
                glm::mat4 localMat;
                for (int i=0; i<16; ++i) localMat[i/4][i%4] = (float)node.matrix[i];
                matrix = matrix * localMat;
            } else {
                glm::mat4 t(1.0f), r(1.0f), s(1.0f);
                if (node.translation.size() == 3) t = glm::translate(glm::mat4(1.0f), glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
                if (node.rotation.size() == 4) {
                    glm::quat q((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
                    r = glm::mat4_cast(q);
                }
                if (node.scale.size() == 3) s = glm::scale(glm::mat4(1.0f), glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
                matrix = matrix * t * r * s;
            }
            
            if (node.mesh > -1) {
                const tinygltf::Mesh& gmesh = gltfModel.meshes[node.mesh];
                for (size_t i = 0; i < gmesh.primitives.size(); ++i) {
                    const tinygltf::Primitive& primitive = gmesh.primitives[i];
                    
                    uint32_t vertexStart = static_cast<uint32_t>(mesh.vertices.size());
                    
                    // Positions
                    const unsigned char* positionDataBytes = nullptr;
                    size_t vertexCount = 0;
                    size_t posStride = 0;
                    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        positionDataBytes = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        vertexCount = accessor.count;
                        posStride = accessor.ByteStride(bufferView);
                    }
                    
                    // Normals
                    const unsigned char* normalDataBytes = nullptr;
                    size_t normStride = 0;
                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        normalDataBytes = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        normStride = accessor.ByteStride(bufferView);
                    }
                    
                    // TexCoords
                    const unsigned char* texcoordDataBytes = nullptr;
                    size_t texStride = 0;
                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        texcoordDataBytes = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                        texStride = accessor.ByteStride(bufferView);
                    }
                    
                    for (size_t v = 0; v < vertexCount; ++v) {
                        Vertex vertex{};
                        if (positionDataBytes) {
                            const float* pos = reinterpret_cast<const float*>(positionDataBytes + v * posStride);
                            glm::vec4 localPos = glm::vec4(pos[0], pos[1], pos[2], 1.0f);
                            vertex.pos = glm::vec3(matrix * localPos);
                        } else {
                            vertex.pos = glm::vec3(0.0f);
                        }
                        
                        if (normalDataBytes) {
                            const float* norm = reinterpret_cast<const float*>(normalDataBytes + v * normStride);
                            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(matrix)));
                            vertex.normal = glm::normalize(normalMatrix * glm::vec3(norm[0], norm[1], norm[2]));
                        } else {
                            vertex.normal = {0.0f, 1.0f, 0.0f};
                        }
                        
                        if (texcoordDataBytes) {
                            const float* tex = reinterpret_cast<const float*>(texcoordDataBytes + v * texStride);
                            vertex.texCoord = {tex[0], tex[1]};
                        } else {
                            vertex.texCoord = {0.0f, 0.0f};
                        }
                        
                        vertex.color = {1.0f, 1.0f, 1.0f};
                        mesh.vertices.push_back(vertex);
                    }
                    
                    // Indices
                    if (primitive.indices > -1) {
                        const tinygltf::Accessor& accessor = gltfModel.accessors[primitive.indices];
                        const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
                        const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
                        
                        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                            const uint32_t* indexData = reinterpret_cast<const uint32_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t ind = 0; ind < accessor.count; ++ind) {
                                mesh.indices.push_back(vertexStart + indexData[ind]);
                            }
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                            const uint16_t* indexData = reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t ind = 0; ind < accessor.count; ++ind) {
                                mesh.indices.push_back(vertexStart + indexData[ind]);
                            }
                        } else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
                            const uint8_t* indexData = reinterpret_cast<const uint8_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                            for (size_t ind = 0; ind < accessor.count; ++ind) {
                                mesh.indices.push_back(vertexStart + indexData[ind]);
                            }
                        }
                    } else {
                        // Non-indexed
                        for (size_t v = 0; v < vertexCount; ++v) {
                            mesh.indices.push_back(static_cast<uint32_t>(vertexStart + v));
                        }
                    }
                }
            }
            
            for (int child : node.children) {
                processNode(child, matrix);
            }
        };
        
        for (int nodeIdx : scene.nodes) {
            processNode(nodeIdx, glm::mat4(1.0f));
        }
        
    } else {
        // Fallback to OBJ
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
            throw std::runtime_error(warn + err);
        
        for (const auto& shape : shapes) {
            for (const auto& idx : shape.mesh.indices) {
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                };
                
                if (idx.texcoord_index >= 0) {
                    vertex.texCoord = {
                        attrib.texcoords[2 * idx.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
                    };
                }
                if (idx.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    };
                }
                vertex.color = {1.0f, 1.0f, 1.0f};

                mesh.vertices.push_back(vertex);
                mesh.indices.push_back(static_cast<uint32_t>(mesh.vertices.size()) - 1);
            }
        }
    }
    
    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
}


// ---- Procedural Mesh Generators ----

int VulkanApp::createCubeMesh()
{
    Mesh mesh;

    // Each face: 4 vertices, 6 indices
    // Normals per face
    struct FaceDef { glm::vec3 normal; glm::vec3 verts[4]; glm::vec3 colors[4]; };
    std::vector<FaceDef> faces = {
        // Front (+Z)
        { {0,0,1}, { {-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f} },
          { {1.0f,0.2f,0.3f},{1.0f,0.5f,0.2f},{1.0f,0.8f,0.2f},{1.0f,0.3f,0.5f} } },
        // Back (-Z)
        { {0,0,-1}, { {0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f,0.5f,-0.5f},{0.5f,0.5f,-0.5f} },
          { {0.1f,0.9f,0.3f},{0.2f,1.0f,0.5f},{0.3f,0.9f,0.7f},{0.1f,0.8f,0.4f} } },
        // Top (-Y in Vulkan)
        { {0,-1,0}, { {-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,-0.5f,0.5f},{-0.5f,-0.5f,0.5f} },
          { {0.1f,0.5f,1.0f},{0.2f,0.6f,1.0f},{0.4f,0.2f,1.0f},{0.1f,0.4f,1.0f} } },
        // Bottom (+Y in Vulkan)
        { {0,1,0}, { {-0.5f,0.5f,0.5f},{0.5f,0.5f,0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f} },
          { {1.0f,0.9f,0.1f},{1.0f,0.7f,0.2f},{1.0f,0.9f,0.3f},{1.0f,0.8f,0.1f} } },
        // Right (+X)
        { {1,0,0}, { {0.5f,-0.5f,0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{0.5f,0.5f,0.5f} },
          { {0.9f,0.2f,0.9f},{0.7f,0.1f,1.0f},{1.0f,0.3f,0.9f},{0.8f,0.2f,1.0f} } },
        // Left (-X)
        { {-1,0,0}, { {-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,0.5f},{-0.5f,0.5f,0.5f},{-0.5f,0.5f,-0.5f} },
          { {0.1f,0.9f,0.9f},{0.2f,1.0f,0.8f},{0.1f,0.8f,1.0f},{0.3f,0.9f,1.0f} } },
    };

    glm::vec2 uvs[4] = { {0,1},{1,1},{1,0},{0,0} };

    for (auto& face : faces) {
        uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
        for (int v = 0; v < 4; v++) {
            Vertex vert{};
            vert.pos      = face.verts[v];
            vert.color    = face.colors[v];
            vert.normal   = face.normal;
            vert.texCoord = uvs[v];
            mesh.vertices.push_back(vert);
        }
        mesh.indices.insert(mesh.indices.end(), {base,base+1,base+2, base+2,base+3,base});
    }

    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int VulkanApp::createSphereMesh(int stacks, int slices)
{
    Mesh mesh;
    const float PI = glm::pi<float>();

    for (int i = 0; i <= stacks; ++i) {
        float phi = PI * float(i) / float(stacks); // 0..PI
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * PI * float(j) / float(slices);
            Vertex v{};
            v.pos = { std::sin(phi)*std::cos(theta)*0.5f,
                     -std::cos(phi)*0.5f,
                      std::sin(phi)*std::sin(theta)*0.5f };
            v.normal   = glm::normalize(v.pos);
            v.color    = { 0.9f + 0.1f*std::sin(theta), 0.7f, 0.3f };
            v.texCoord = { float(j)/float(slices), float(i)/float(stacks) };
            mesh.vertices.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t r0 = (uint32_t)((i  )*(slices+1) + j);
            uint32_t r1 = (uint32_t)((i+1)*(slices+1) + j);
            mesh.indices.insert(mesh.indices.end(), {r0, r1, r0+1, r1, r1+1, r0+1});
        }
    }

    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int VulkanApp::createPlaneMesh()
{
    Mesh mesh;
    // A flat plane on XZ, centered at origin, size 1x1
    glm::vec3 normal = {0, 1, 0};
    glm::vec3 cols[4] = {
        {0.35f,0.35f,0.4f}, {0.3f,0.3f,0.38f},
        {0.4f,0.4f,0.45f},  {0.32f,0.32f,0.4f}
    };
    glm::vec3 pos[4] = {
        {-0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, -0.5f},
        {0.5f,  0.0f,  0.5f}, {-0.5f, 0.0f, 0.5f}
    };
    glm::vec2 uvs[4] = { {0,0},{1,0},{1,1},{0,1} };
    for (int i = 0; i < 4; i++) {
        Vertex v{};
        v.pos = pos[i]; v.normal = normal; v.color = cols[i]; v.texCoord = uvs[i];
        mesh.vertices.push_back(v);
    }
    mesh.indices = {0,2,1, 0,3,2};
    mesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

void VulkanApp::initVulkan()
{
    createInstance();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createShadowRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createShadowResources();
    createShadowPipeline();
    createDepthResources();
    createFramebuffers();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createOffscreenResources(); // Add offscreen resources
    createDefaultTexture();     // Default 1x1 white texture
    createSyncObjects();
}

void VulkanApp::createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Unity Hub Vulkan Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}

void VulkanApp::createSurface()
{
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void VulkanApp::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    int bestScore = -1;
    for (const auto& dev : devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(dev, &properties);

        bool suitable = isDeviceSuitable(dev);
        int score = suitable ? rateDeviceSuitability(dev, properties) : -1;

        if (suitable && score > bestScore)
        {
            physicalDevice = dev;
            bestScore = score;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties selectedProps;
    vkGetPhysicalDeviceProperties(physicalDevice, &selectedProps);
    selectedGpuName = selectedProps.deviceName;
}

bool VulkanApp::isDeviceSuitable(VkPhysicalDevice dev)
{
    QueueFamilyIndices indices = findQueueFamilies(dev);
    bool extensionsSupported = checkDeviceExtensionSupport(dev);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice dev)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice dev)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);

        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.isComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice dev)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void VulkanApp::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR VulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void VulkanApp::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanApp::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

VkImageView VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image view!");
    }

    return imageView;
}

void VulkanApp::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
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

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void VulkanApp::createDescriptorSetLayout()
{
    // UBO Layout (Set 0)
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &uboLayoutInfo, nullptr, &uboSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create UBO descriptor set layout!");
    }
    
    // Texture Layout (Set 1)
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo texLayoutInfo{};
    texLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    texLayoutInfo.bindingCount = 1;
    texLayoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &texLayoutInfo, nullptr, &textureSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Texture descriptor set layout!");
    }
}

void VulkanApp::createGraphicsPipeline()
{
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;

    try
    {
        vertShaderCode = readFile("vert.spv");
        fragShaderCode = readFile("frag.spv");
    }
    catch (...)
    {
        vertShaderCode = readFile("shaders/vert.spv");
        fragShaderCode = readFile("shaders/frag.spv");
    }

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    std::array<VkDescriptorSetLayout, 2> setLayouts = {uboSetLayout, textureSetLayout};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = setLayouts.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

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
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = offscreenRenderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule VulkanApp::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

void VulkanApp::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void VulkanApp::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

VkFormat VulkanApp::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

VkFormat VulkanApp::findDepthFormat()
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void VulkanApp::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    createImage(
        swapChainExtent.width,
        swapChainExtent.height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        depthImage,
        depthImageMemory
    );

    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void VulkanApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

void VulkanApp::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanApp::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertexBufferMemory
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanApp::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory
    );

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanApp::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Scene camera UBOs
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    // Game camera UBOs
    gameUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    gameUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    gameUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffers[i], uniformBuffersMemory[i]);
        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);

        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            gameUniformBuffers[i], gameUniformBuffersMemory[i]);
        vkMapMemory(device, gameUniformBuffersMemory[i], 0, bufferSize, 0, &gameUniformBuffersMapped[i]);
    }
}

void VulkanApp::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1000; // Allow up to 1000 textures

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2 + 1000; // scene+game frames + textures

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void VulkanApp::createDescriptorSets()
{
    // --- Scene camera descriptor sets ---
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, uboSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate scene descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{ uniformBuffers[i], 0, sizeof(UniformBufferObject) };
        VkWriteDescriptorSet dw{};
        dw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dw.dstSet = descriptorSets[i];
        dw.dstBinding = 0;
        dw.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dw.descriptorCount = 1;
        dw.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(device, 1, &dw, 0, nullptr);
    }

    // --- Game camera descriptor sets ---
    gameDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, gameDescriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate game descriptor sets!");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{ gameUniformBuffers[i], 0, sizeof(UniformBufferObject) };
        VkWriteDescriptorSet dw{};
        dw.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        dw.dstSet = gameDescriptorSets[i];
        dw.dstBinding = 0;
        dw.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dw.descriptorCount = 1;
        dw.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(device, 1, &dw, 0, nullptr);
    }
}

void VulkanApp::setupUnityStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 0.94f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.16f, 0.17f, 0.20f, 0.80f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.15f, 0.16f, 0.19f, 0.96f);
    colors[ImGuiCol_Border]                = ImVec4(0.24f, 0.26f, 0.30f, 0.60f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.28f, 0.30f, 0.36f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.34f, 0.36f, 0.42f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.12f, 0.13f, 0.15f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.26f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.34f, 0.36f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.42f, 0.50f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.00f, 0.58f, 0.96f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.58f, 0.96f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.18f, 0.68f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.22f, 0.24f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.52f, 0.88f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.44f, 0.76f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.52f, 0.88f, 0.80f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.44f, 0.76f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.25f, 0.27f, 0.32f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.00f, 0.52f, 0.88f, 0.80f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.22f, 0.25f, 0.30f, 1.00f);
}

void VulkanApp::initImGui()
{
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create ImGui descriptor pool!");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    setupUnityStyle();

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = indices.graphicsFamily.value();
    init_info.Queue = graphicsQueue;
    init_info.DescriptorPool = imguiDescriptorPool;
    init_info.RenderPass = renderPass;
    init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = static_cast<uint32_t>(swapChainImages.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info);

    // Scene view texture
    offscreenDescriptorSet = ImGui_ImplVulkan_AddTexture(
        offscreenSampler,
        offscreenColorImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );

    // Game view texture (uses Main Camera)
    gameViewDescriptorSet = ImGui_ImplVulkan_AddTexture(
        gameSampler,
        gameColorImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
}

void VulkanApp::updateProfilerMetrics(float deltaTime)
{
    static double lastCpuCheckTime = 0.0;
    double curTime = glfwGetTime();

    // 1. FrameTime & FPS
    float frameTimeMs = deltaTime * 1000.0f;
    float fps = ImGui::GetIO().Framerate;

    profilerMetrics.frameTimeMs = frameTimeMs;
    profilerMetrics.fps = fps;

    std::rotate(profilerMetrics.frameTimeHistory.begin(), profilerMetrics.frameTimeHistory.begin() + 1, profilerMetrics.frameTimeHistory.end());
    profilerMetrics.frameTimeHistory.back() = frameTimeMs;

    float sum = 0.0f;
    profilerMetrics.minFrameTime = 999.0f;
    profilerMetrics.maxFrameTime = 0.0f;
    for (float f : profilerMetrics.frameTimeHistory)
    {
        if (f > 0.001f)
        {
            if (f < profilerMetrics.minFrameTime) profilerMetrics.minFrameTime = f;
            if (f > profilerMetrics.maxFrameTime) profilerMetrics.maxFrameTime = f;
            sum += f;
        }
    }
    profilerMetrics.avgFrameTime = sum / static_cast<float>(profilerMetrics.frameTimeHistory.size());

    // 2. RAM Usage (Win32 Working Set Size)
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        profilerMetrics.ramUsageMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
    }
#endif
    std::rotate(profilerMetrics.ramHistory.begin(), profilerMetrics.ramHistory.begin() + 1, profilerMetrics.ramHistory.end());
    profilerMetrics.ramHistory.back() = profilerMetrics.ramUsageMB;

    // 3. CPU Usage (%)
#ifdef _WIN32
    if (curTime - lastCpuCheckTime >= 0.2)
    {
        lastCpuCheckTime = curTime;
        static FILETIME prevSysKernel, prevSysUser, prevProcKernel, prevProcUser;
        static bool firstCall = true;

        FILETIME sysIdle, sysKernel, sysUser;
        FILETIME procCreation, procExit, procKernel, procUser;

        if (GetSystemTimes(&sysIdle, &sysKernel, &sysUser) &&
            GetProcessTimes(GetCurrentProcess(), &procCreation, &procExit, &procKernel, &procUser))
        {
            if (!firstCall)
            {
                uint64_t sysKernelDiff = ((uint64_t)sysKernel.dwHighDateTime << 32 | sysKernel.dwLowDateTime) -
                                         ((uint64_t)prevSysKernel.dwHighDateTime << 32 | prevSysKernel.dwLowDateTime);
                uint64_t sysUserDiff = ((uint64_t)sysUser.dwHighDateTime << 32 | sysUser.dwLowDateTime) -
                                       ((uint64_t)prevSysUser.dwHighDateTime << 32 | prevSysUser.dwLowDateTime);

                uint64_t procKernelDiff = ((uint64_t)procKernel.dwHighDateTime << 32 | procKernel.dwLowDateTime) -
                                           ((uint64_t)prevProcKernel.dwHighDateTime << 32 | prevProcKernel.dwLowDateTime);
                uint64_t procUserDiff = ((uint64_t)procUser.dwHighDateTime << 32 | procUser.dwLowDateTime) -
                                         ((uint64_t)prevProcUser.dwHighDateTime << 32 | prevProcUser.dwLowDateTime);

                uint64_t totalSys = sysKernelDiff + sysUserDiff;
                uint64_t totalProc = procKernelDiff + procUserDiff;

                if (totalSys > 0)
                {
                    SYSTEM_INFO sysInfo;
                    GetSystemInfo(&sysInfo);
                    int numCores = sysInfo.dwNumberOfProcessors > 0 ? sysInfo.dwNumberOfProcessors : 1;
                    float cpuPct = (float)((double)totalProc / (double)totalSys) * 100.0f * numCores;
                    profilerMetrics.cpuUsagePercent = std::clamp(cpuPct, 0.0f, 100.0f);
                }
            }

            prevSysKernel = sysKernel;
            prevSysUser = sysUser;
            prevProcKernel = procKernel;
            prevProcUser = procUser;
            firstCall = false;
        }
    }
#endif
    std::rotate(profilerMetrics.cpuHistory.begin(), profilerMetrics.cpuHistory.begin() + 1, profilerMetrics.cpuHistory.end());
    profilerMetrics.cpuHistory.back() = profilerMetrics.cpuUsagePercent;

    // 4. VRAM Usage (Calculated Vulkan Buffer & Texture Memory)
    size_t vramBytes = 0;
    for (const auto& mesh : meshes)
    {
        vramBytes += mesh.vertices.size() * sizeof(Vertex);
        vramBytes += mesh.indices.size() * sizeof(uint32_t);
    }
    int fbW = WIDTH, fbH = HEIGHT;
    if (window) glfwGetFramebufferSize(window, &fbW, &fbH);
    vramBytes += fbW * fbH * 4 * 2; // Offscreen Color & Game Color
    vramBytes += fbW * fbH * 4 * 2; // Offscreen Depth & Game Depth
    vramBytes += 2048 * 2048 * 4;   // Shadow Map
    vramBytes += swapChainImages.size() * fbW * fbH * 4;
    for (size_t i = 0; i < textures.size(); ++i)
    {
        vramBytes += 1024 * 1024 * 4;
    }
    profilerMetrics.vramUsageMB = static_cast<float>(vramBytes) / (1024.0f * 1024.0f);

    std::rotate(profilerMetrics.vramHistory.begin(), profilerMetrics.vramHistory.begin() + 1, profilerMetrics.vramHistory.end());
    profilerMetrics.vramHistory.back() = profilerMetrics.vramUsageMB;
}

void VulkanApp::drawProfilerPanel()
{
    if (!showProfilerPanel) return;

    ImGui::SetNextWindowSize(ImVec2(340, 480), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("📊 Visual Profiler Panel", &showProfilerPanel, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

        // 1. Performance Summary
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "⚙️ Performance Metrics");
        ImGui::Separator();

        ImVec4 fpsCol = (profilerMetrics.fps >= 55.0f) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) :
                        (profilerMetrics.fps >= 30.0f) ? ImVec4(0.9f, 0.8f, 0.2f, 1.0f) :
                                                         ImVec4(0.9f, 0.2f, 0.2f, 1.0f);

        ImGui::Text("Frames / Sec :"); ImGui::SameLine();
        ImGui::TextColored(fpsCol, "%.1f FPS", profilerMetrics.fps);

        ImGui::Text("Frame Time  :"); ImGui::SameLine();
        ImGui::TextColored(fpsCol, "%.2f ms", profilerMetrics.frameTimeMs);
        ImGui::TextDisabled("  (Min: %.2f ms | Max: %.2f ms | Avg: %.2f ms)",
                            profilerMetrics.minFrameTime, profilerMetrics.maxFrameTime, profilerMetrics.avgFrameTime);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 2. Hardware Resources
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "💻 Hardware Resources");

        ImVec4 cpuCol = (profilerMetrics.cpuUsagePercent < 50.0f) ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) :
                        (profilerMetrics.cpuUsagePercent < 80.0f) ? ImVec4(0.9f, 0.8f, 0.2f, 1.0f) :
                                                                    ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
        ImGui::Text("CPU Usage   :"); ImGui::SameLine();
        ImGui::TextColored(cpuCol, "%.1f %%", profilerMetrics.cpuUsagePercent);

        ImGui::Text("RAM (System):"); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%.1f MB", profilerMetrics.ramUsageMB);

        ImGui::Text("VRAM (Vulkan):"); ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "%.1f MB", profilerMetrics.vramUsageMB);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 3. Realtime History Graphs
        if (ImGui::CollapsingHeader("📈 Realtime History Graphs", ImGuiTreeNodeFlags_DefaultOpen))
        {
            char ftLabel[64];
            snprintf(ftLabel, sizeof(ftLabel), "Frame Time (%.1f ms)", profilerMetrics.frameTimeMs);
            ImGui::PlotLines("##FrameTimePlot", profilerMetrics.frameTimeHistory.data(),
                             static_cast<int>(profilerMetrics.frameTimeHistory.size()),
                             0, ftLabel, 0.0f, 40.0f, ImVec2(-1, 55));

            char cpuLabel[64];
            snprintf(cpuLabel, sizeof(cpuLabel), "CPU Usage (%.1f %%)", profilerMetrics.cpuUsagePercent);
            ImGui::PlotLines("##CPUPlot", profilerMetrics.cpuHistory.data(),
                             static_cast<int>(profilerMetrics.cpuHistory.size()),
                             0, cpuLabel, 0.0f, 100.0f, ImVec2(-1, 55));

            char memLabel[64];
            snprintf(memLabel, sizeof(memLabel), "RAM: %.1f MB | VRAM: %.1f MB", profilerMetrics.ramUsageMB, profilerMetrics.vramUsageMB);
            ImGui::PlotLines("##RAMPlot", profilerMetrics.ramHistory.data(),
                             static_cast<int>(profilerMetrics.ramHistory.size()),
                             0, memLabel, 0.0f, 500.0f, ImVec2(-1, 55));
        }

        ImGui::Spacing();
        if (ImGui::CollapsingHeader("🎮 Vulkan Engine Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Render Device : %s", selectedGpuName.c_str());
            ImGui::Text("Scene Objects : %d", static_cast<int>(sceneObjects.size()));
            ImGui::Text("Loaded Meshes : %d", static_cast<int>(meshes.size()));
            ImGui::Text("Shadow Mapping: %s (2048x2048)", enableShadowMapping ? "Enabled" : "Disabled");
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();
}

int VulkanApp::load3DModelAsset(const std::string& filePath)
{
    try {
        Mesh newMesh;
        loadModel(filePath, newMesh);
        createMeshBuffers(newMesh);
        meshes.push_back(newMesh);
        return static_cast<int>(meshes.size()) - 1;
    }
    catch (const std::exception& e) {
        printf("Error loading 3D Model Asset (%s): %s\n", filePath.c_str(), e.what());
        return -1;
    }
}

int VulkanApp::loadTextureAsset(const std::string& filePath)
{
    try {
        Texture newTexture;
        loadTexture(filePath, newTexture);
        textures.push_back(newTexture);
        return static_cast<int>(textures.size()) - 1;
    }
    catch (const std::exception& e) {
        printf("Error loading Texture Asset (%s): %s\n", filePath.c_str(), e.what());
        return -1;
    }
}

void VulkanApp::drawAssetBrowserPanel(float windowWidth, float bottomBarHeight)
{
    if (!showAssetBrowserPanel) return;

    ImGui::SetNextWindowPos(ImVec2(0.0f, ImGui::GetIO().DisplaySize.y - bottomBarHeight));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, bottomBarHeight));

    if (ImGui::Begin("📁 Project Assets & Drag-and-Drop Browser", &showAssetBrowserPanel, ImGuiWindowFlags_NoCollapse))
    {
        if (ImGui::BeginTabBar("AssetBrowserTabs"))
        {
            if (ImGui::BeginTabItem("📦 Workspace Assets (assets/)"))
            {
                std::string assetsDir = "assets";
                if (!std::filesystem::exists(assetsDir))
                {
                    std::filesystem::create_directory(assetsDir);
                }

                float cardWidth = 110.0f;
                float cardHeight = 92.0f;
                float availWidth = ImGui::GetContentRegionAvail().x;
                int cols = static_cast<int>(availWidth / (cardWidth + 12.0f));
                if (cols < 1) cols = 1;

                if (ImGui::BeginTable("AssetsGrid", cols, ImGuiTableFlags_SizingFixedFit))
                {
                    int col = 0;
                    for (const auto& entry : std::filesystem::directory_iterator(assetsDir))
                    {
                        if (entry.is_regular_file())
                        {
                            std::string pathStr = entry.path().string();
                            std::string filenameStr = entry.path().filename().string();
                            std::string ext = entry.path().extension().string();
                            for (auto& c : ext) c = tolower(c);

                            if (col == 0) ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(col);

                            ImGui::PushID(pathStr.c_str());

                            ImGui::BeginGroup();
                            ImVec2 p = ImGui::GetCursorScreenPos();
                            ImDrawList* drawList = ImGui::GetWindowDrawList();

                            // Card box background
                            bool isHovered = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + cardWidth, p.y + cardHeight));
                            ImU32 bgCol = isHovered ? IM_COL32(45, 50, 65, 255) : IM_COL32(30, 32, 38, 255);
                            ImU32 borderCol = isHovered ? IM_COL32(0, 180, 255, 255) : IM_COL32(60, 65, 75, 255);
                            drawList->AddRectFilled(p, ImVec2(p.x + cardWidth, p.y + cardHeight), bgCol, 6.0f);
                            drawList->AddRect(p, ImVec2(p.x + cardWidth, p.y + cardHeight), borderCol, 6.0f, 0, isHovered ? 1.8f : 1.0f);

                            ImGui::Dummy(ImVec2(cardWidth, cardHeight));

                            // Selectable dummy for mouse clicks & drag
                            ImGui::SetCursorScreenPos(p);
                            ImGui::Selectable("##CardSelect", false, 0, ImVec2(cardWidth, cardHeight));

                            // Drag & Drop Source attached directly to the full card Selectable item
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                            {
                                if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_MODEL", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("📦 Dragging 3D Model '%s'\nDrop into Scene View to place object!", filenameStr.c_str());
                                }
                                else if (ext == ".lua")
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_LUA", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("📖 Dragging Lua Script '%s'\nDrop into Inspector / Hierarchy to attach script!", filenameStr.c_str());
                                }
                                else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_TEXTURE", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("🖼️ Dragging Texture '%s'\nDrop onto Object in Scene / Inspector to apply!", filenameStr.c_str());
                                }
                                else
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("📄 Dragging File '%s'", filenameStr.c_str());
                                }
                                ImGui::EndDragDropSource();
                            }

                            // Double Click Action on full card Selectable
                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            {
                                if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
                                {
                                    saveHistory();
                                    int meshId = load3DModelAsset(pathStr);
                                    if (meshId >= 0)
                                    {
                                        SceneObject newObj;
                                        newObj.id = sceneObjects.size();
                                        newObj.name = entry.path().stem().string();
                                        newObj.position = glm::vec3(0.0f);
                                        newObj.scale = glm::vec3(1.0f);
                                        newObj.color = glm::vec4(1.0f);
                                        newObj.meshId = meshId;
                                        sceneObjects.push_back(newObj);
                                        selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
                                    }
                                }
                                else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                                {
                                    saveHistory();
                                    int texId = loadTextureAsset(pathStr);
                                    if (texId >= 0 && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                                    {
                                        sceneObjects[selectedObjectIndex].textureId = texId;
                                    }
                                }
                                else if (ext == ".lua")
                                {
                                    if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                                    {
                                        std::ifstream t(pathStr);
                                        if (t.is_open())
                                        {
                                            std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                                            sceneObjects[selectedObjectIndex].luaScripts.push_back(scriptContent);
                                        }
                                    }
                                }
                            }

                            // Icon / Thumbnail visuals
                            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                            {
                                if (assetThumbnails.find(pathStr) == assetThumbnails.end())
                                {
                                    try {
                                        Texture tex;
                                        loadTexture(pathStr, tex);
                                        assetThumbnails[pathStr] = tex;
                                    } catch (...) {}
                                }

                                if (assetThumbnails.find(pathStr) != assetThumbnails.end())
                                {
                                    ImGui::SetCursorScreenPos(ImVec2(p.x + (cardWidth - 44.0f) * 0.5f, p.y + 8.0f));
                                    ImGui::Image((ImTextureID)assetThumbnails[pathStr].descriptorSet, ImVec2(44.0f, 44.0f));
                                }
                                else
                                {
                                    ImGui::SetCursorScreenPos(ImVec2(p.x + 28.0f, p.y + 12.0f));
                                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "🖼️ TEX");
                                }
                            }
                            else if (ext == ".lua")
                            {
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 42.0f, p.y + 10.0f));
                                ImGui::SetWindowFontScale(1.3f);
                                ImGui::Text("📖");
                                ImGui::SetWindowFontScale(1.0f);
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 18.0f, p.y + 36.0f));
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "LUA SCRIPT");
                            }
                            else if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
                            {
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 42.0f, p.y + 10.0f));
                                ImGui::SetWindowFontScale(1.3f);
                                ImGui::Text("📦");
                                ImGui::SetWindowFontScale(1.0f);
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 22.0f, p.y + 36.0f));
                                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "3D MODEL");
                            }
                            else
                            {
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 42.0f, p.y + 14.0f));
                                ImGui::Text("📄");
                            }

                            // Filename Label at bottom of card
                            std::string displayFilename = filenameStr;
                            if (displayFilename.size() > 11) displayFilename = displayFilename.substr(0, 9) + "..";
                            ImGui::SetCursorScreenPos(ImVec2(p.x + 6.0f, p.y + cardHeight - 20.0f));
                            ImGui::EndGroup();
                            ImGui::PopID();
                            col = (col + 1) % cols;
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("🎲 Built-in Primitives"))
            {
                struct PrimitiveItem {
                    const char* name;
                    const char* payloadKey;
                    const char* icon;
                };

                PrimitiveItem primitives[] = {
                    { "Cube Primitive", "PRIMITIVE_CUBE", "🎲" },
                    { "Sphere Primitive", "PRIMITIVE_SPHERE", "🔮" },
                    { "Plane Primitive", "PRIMITIVE_PLANE", "📜" }
                };

                for (const auto& item : primitives)
                {
                    std::string label = std::string(item.icon) + " " + item.name;
                    ImGui::Selectable(label.c_str(), false, 0, ImVec2(180, 26));

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        ImGui::SetDragDropPayload("DND_ASSET_MODEL", item.payloadKey, strlen(item.payloadKey) + 1);
                        ImGui::Text("🎲 Dragging '%s'\nDrop into Scene View to place!", item.name);
                        ImGui::EndDragDropSource();
                    }

                    ImGui::SameLine();
                }
                ImGui::NewLine();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void VulkanApp::renderImGuiUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Handle editor hotkeys for gizmo switching (Q = Hand, W = Translate, E = Rotate, R = Scale, T = Rect, Y = Combined)
    if (mode == AppMode::EDIT)
    {
        // Undo / Redo
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
        {
            undo();
        }
        if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            redo();
        }

        // Delete Entity hotkey (Delete / Backspace)
        if (!ImGui::GetIO().WantTextInput && (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)))
        {
            if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
            {
                saveHistory();
                sceneObjects.erase(sceneObjects.begin() + selectedObjectIndex);
                selectedObjectIndex = -1;
            }
        }
        
        if (ImGui::IsKeyPressed(ImGuiKey_Q))
        {
            activeGizmo = GizmoType::HAND;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_W))
        {
            activeGizmo = GizmoType::TRANSLATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_E))
        {
            activeGizmo = GizmoType::ROTATE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R))
        {
            activeGizmo = GizmoType::SCALE;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_T))
        {
            activeGizmo = GizmoType::RECT;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Y))
        {
            activeGizmo = GizmoType::TRANSFORM_COMBINED;
        }
    }

    // Get current GLFW window size for dynamic tiling layout
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float windowWidth = static_cast<float>(width);
    float windowHeight = static_cast<float>(height);

    float menuBarHeight = 25.0f;
    float bottomBarHeight = 95.0f;
    float leftPanelWidth = 260.0f;
    float rightPanelWidth = 330.0f;
    float centerWidth = windowWidth - leftPanelWidth - rightPanelWidth;
    float centerHeight = windowHeight - menuBarHeight - bottomBarHeight;

    if (centerWidth < 100.0f) centerWidth = 100.0f;
    if (centerHeight < 100.0f) centerHeight = 100.0f;

    // 1. Top Menu Bar with File Save/Load and Play/Edit Controls
    if (ImGui::BeginMainMenuBar())
    {
        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.0f, 1.0f), "  ⚙️  ANTIGRAVITY ENGINE");
        ImGui::Separator();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene"))
            {
                initializeDefaultScene();
                gameScore = 0;
            }
            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
            {
                saveScene("scene.json");
            }
            if (ImGui::MenuItem("Load Scene", "Ctrl+L"))
            {
                loadScene("scene.json");
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();

        if (mode == AppMode::PLAY)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.65f, 0.32f, 1.0f));
            if (ImGui::Button(" [ > PLAYING ] "))
            {
                mode = AppMode::EDIT;
                updateWindowTitle();
                restoreEditModeState();
                for (auto& obj : sceneObjects)
                    obj.luaInstances.clear();
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            if (ImGui::Button(" [ STOP ] "))
            {
                mode = AppMode::EDIT;
                updateWindowTitle();
                restoreEditModeState();
                for (auto& obj : sceneObjects)
                    obj.luaInstances.clear();
            }
        }
        else
        {
            if (ImGui::Button(" [ PLAY ] "))
            {
                mode = AppMode::PLAY;
                updateWindowTitle();
                savePlayModeState();
                initPhysicsBodies();
                // Init ALL Lua Scripts (multi-script per object)
                for (auto& obj : sceneObjects)
                {
                    obj.luaInstances.clear();
                    for (const auto& script : obj.luaScripts)
                    {
                        if (script.empty()) continue;
                        try {
                            sol::protected_function_result result = luaState.script(script);
                            if (result.valid() && result.get_type() == sol::type::table) {
                                sol::table instance = result;
                                obj.luaInstances.push_back(instance);
                                sol::protected_function onStart = instance["onStart"];
                                if (onStart.valid()) {
                                    auto res = onStart(instance, &obj);
                                    if (!res.valid()) {
                                        sol::error err = res;
                                        printf("Lua onStart error: %s\n", err.what());
                                    }
                                }
                            }
                        } catch (const sol::error& e) {
                            printf("Lua syntax error: %s\n", e.what());
                        }
                    }
                }
                gameScore = 0;
            }
        }

        ImGui::Separator();
        if (ImGui::Button(showGameViewWindow ? " [ [x] Game View Window ] " : " [ [ ] Game View Window ] "))
        {
            showGameViewWindow = !showGameViewWindow;
        }

        ImGui::Separator();
        if (ImGui::Button(showProfilerPanel ? " [ [x] 📊 Visual Profiler ] " : " [ [ ] 📊 Visual Profiler ] "))
        {
            showProfilerPanel = !showProfilerPanel;
        }

        ImGui::Separator();
        if (ImGui::Button(showAssetBrowserPanel ? " [ [x] 📁 Asset Browser ] " : " [ [ ] 📁 Asset Browser ] "))
        {
            showAssetBrowserPanel = !showAssetBrowserPanel;
        }

        ImGui::Separator();
        ImGui::Text("GPU: %s", selectedGpuName.c_str());

        ImGui::Separator();
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

        ImGui::EndMainMenuBar();
    }

    // 2. Hierarchy Panel (Left Window)
    ImGui::SetNextWindowPos(ImVec2(0.0f, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, centerHeight));
    if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        // Add Object button
        if (ImGui::Button(" + Create ", ImVec2(-1, 25)))
        {
            ImGui::OpenPopup("CreateObjectPopup");
        }
        
        if (ImGui::BeginPopup("CreateObjectPopup"))
        {
            if (ImGui::MenuItem("3D Cube"))
            {
                saveHistory();
                SceneObject newObj;
                newObj.id = sceneObjects.size();
                newObj.name = "Cube " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::CUBE;
                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);
                newObj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                newObj.scale = glm::vec3(0.5f, 0.5f, 0.5f);
                newObj.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                newObj.isPhysicsEnabled = false;
                newObj.meshId = primitiveCubeMeshId;
                sceneObjects.push_back(newObj);
                selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
            }
            if (ImGui::MenuItem("3D Sphere"))
            {
                saveHistory();
                SceneObject newObj;
                newObj.id = sceneObjects.size();
                newObj.name = "Sphere " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::SPHERE;
                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);
                newObj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                newObj.scale = glm::vec3(0.5f, 0.5f, 0.5f);
                newObj.color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
                newObj.isPhysicsEnabled = false;
                newObj.meshId = primitiveSphereMeshId;
                sceneObjects.push_back(newObj);
                selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
            }
            if (ImGui::MenuItem("Directional Light"))
            {
                saveHistory();
                SceneObject newObj;
                newObj.id = sceneObjects.size();
                newObj.name = "Light " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::LIGHT;
                newObj.position = glm::vec3(0.0f, 2.0f, 0.0f);
                newObj.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
                newObj.scale = glm::vec3(0.3f, 0.3f, 0.3f);
                newObj.color = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);
                newObj.isPhysicsEnabled = false;
                newObj.meshId = primitiveCubeMeshId;
                sceneObjects.push_back(newObj);
                selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::TreeNodeEx("SampleScene", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // List all objects in the scene database
            for (size_t i = 0; i < sceneObjects.size(); ++i)
            {
                std::string icon = "🧊 ";
                if (sceneObjects[i].type == ObjectType::SPHERE) icon = "🟡 ";
                if (sceneObjects[i].type == ObjectType::PLANE) icon = "🟩 ";
                if (sceneObjects[i].type == ObjectType::LIGHT) icon = "💡 ";
                
                std::string label = icon + sceneObjects[i].name;
                
                if (ImGui::Selectable(label.c_str(), selectedObjectIndex == static_cast<int>(i)))
                {
                    selectedObjectIndex = static_cast<int>(i);
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payloadLua = ImGui::AcceptDragDropPayload("DND_ASSET_LUA"))
                    {
                        const char* assetPath = static_cast<const char*>(payloadLua->Data);
                        std::ifstream t(assetPath);
                        if (t.is_open())
                        {
                            std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                            sceneObjects[i].luaScripts.push_back(scriptContent);
                            selectedObjectIndex = static_cast<int>(i);
                        }
                    }
                    else if (const ImGuiPayload* payloadTex = ImGui::AcceptDragDropPayload("DND_ASSET_TEXTURE"))
                    {
                        const char* assetPath = static_cast<const char*>(payloadTex->Data);
                        int texId = loadTextureAsset(assetPath);
                        if (texId >= 0)
                        {
                            sceneObjects[i].textureId = texId;
                            selectedObjectIndex = static_cast<int>(i);
                        }
                    }
                    else if (const ImGuiPayload* payloadModel = ImGui::AcceptDragDropPayload("DND_ASSET_MODEL"))
                    {
                        const char* assetPath = static_cast<const char*>(payloadModel->Data);
                        int meshId = load3DModelAsset(assetPath);
                        if (meshId >= 0)
                        {
                            sceneObjects[i].meshId = meshId;
                            selectedObjectIndex = static_cast<int>(i);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Right-Click Context Menu for Hierarchy Items
                std::string contextPopupId = "EntityContextMenu_" + std::to_string(i);
                if (ImGui::BeginPopupContextItem(contextPopupId.c_str()))
                {
                    selectedObjectIndex = static_cast<int>(i);
                    ImGui::TextDisabled("Entity: %s", sceneObjects[i].name.c_str());
                    ImGui::Separator();
                    
                    if (ImGui::MenuItem("📋 Duplicate Entity"))
                    {
                        saveHistory();
                        SceneObject dup = sceneObjects[i];
                        dup.id = sceneObjects.size();
                        dup.name = sceneObjects[i].name + " (Copy)";
                        dup.position += glm::vec3(0.5f, 0.0f, 0.5f);
                        sceneObjects.push_back(dup);
                        selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
                    }

                    if (ImGui::MenuItem("🗑️ Delete Entity"))
                    {
                        saveHistory();
                        sceneObjects.erase(sceneObjects.begin() + i);
                        selectedObjectIndex = -1;
                        ImGui::EndPopup();
                        break;
                    }
                    ImGui::EndPopup();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Special Camera Node
            if (ImGui::Selectable("🎥 Main Camera", selectedObjectIndex == -1))
            {
                selectedObjectIndex = -1; // -1 represents Main Camera selection
            }

            ImGui::TreePop();
        }
    }
    ImGui::End();

    // 3. Scene View (Center-Left Window)
    ImGui::SetNextWindowPos(ImVec2(leftPanelWidth, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(showGameViewWindow ? centerWidth * 0.5f : centerWidth, centerHeight));
    if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
    {
        drawSceneView(ImGui::GetWindowPos(), ImGui::GetWindowSize());
    }
    ImGui::End();

    // 4. Game View (Center-Right Window)
    if (showGameViewWindow)
    {
        ImGui::SetNextWindowPos(ImVec2(leftPanelWidth + centerWidth * 0.5f, menuBarHeight));
        ImGui::SetNextWindowSize(ImVec2(centerWidth * 0.5f, centerHeight));
        if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))
        {
            drawGameView(ImGui::GetWindowPos(), ImGui::GetWindowSize());
        }
        ImGui::End();
    }

    // 5. Inspector Panel (Right Window)
    ImGui::SetNextWindowPos(ImVec2(windowWidth - rightPanelWidth, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, centerHeight));
    if (ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
        
        static char nameBuf[64] = "";
        static char tagBuf[64] = "Untagged";
        static char layerBuf[64] = "Default";
        static bool isStatic = false;

        if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
        {
            auto& obj = sceneObjects[selectedObjectIndex];
            obj.syncComponents();
            
            // Name Header
            ImGui::Text("🔍 Entity (GameObject): ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%s (ID: %d)", obj.name.c_str(), obj.id);
            ImGui::Separator();

            // Properties
            snprintf(nameBuf, sizeof(nameBuf), "%s", obj.name.c_str());
            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
            {
                obj.name = nameBuf;
            }

            ImGui::Checkbox("Static", &isStatic);
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::Text("Tag:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            ImGui::InputText("##Tag", tagBuf, sizeof(tagBuf));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- COMPONENT-BASED INSPECTOR RENDERING ---

            // 1. Transform Component (Always present on Entity)
            if (ImGui::CollapsingHeader("📌 Transform Component", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position (X,Y,Z)", &obj.position.x, 0.05f);
                ImGui::DragFloat3("Rotation (X,Y,Z)", &obj.rotation.x, 0.5f);
                ImGui::DragFloat3("Scale (X,Y,Z)", &obj.scale.x, 0.02f, 0.01f, 10.0f);
            }
            ImGui::Spacing();

            // 2. Mesh Renderer Component
            if (obj.hasComponent(ComponentType::MESH_RENDERER))
            {
                if (ImGui::CollapsingHeader("🧊 Mesh Renderer Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::ColorEdit4("Mesh Color", &obj.color.x);
                    
                    ImGui::Spacing();
                    if (ImGui::Button("Load 3D Mesh (.obj, .glb, .gltf)", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    {
                        const char* filterPatterns[3] = { "*.obj", "*.glb", "*.gltf" };
                        const char* filePath = tinyfd_openFileDialog("Load 3D Model", "", 3, filterPatterns, "3D Model files", 0);
                        if (filePath)
                        {
                            try {
                                Mesh newMesh;
                                loadModel(filePath, newMesh);
                                createMeshBuffers(newMesh);
                                meshes.push_back(newMesh);
                                obj.meshId = static_cast<int>(meshes.size()) - 1;
                            }
                            catch (const std::exception& e) {
                                tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                            }
                        }
                    }
                    
                    if (ImGui::Button("Load Material Texture", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
                    {
                        const char* filterPatterns[2] = { "*.png", "*.jpg" };
                        const char* filePath = tinyfd_openFileDialog("Load Texture", "", 2, filterPatterns, "Image Files", 0);
                        if (filePath)
                        {
                            try {
                                Texture newTexture;
                                loadTexture(filePath, newTexture);
                                textures.push_back(newTexture);
                                obj.textureId = static_cast<int>(textures.size()) - 1;
                            }
                            catch (const std::exception& e) {
                                tinyfd_messageBox("Error", e.what(), "ok", "error", 1);
                            }
                        }
                    }

                    if (obj.name != "Player Cube" && obj.name != "Gold Collectible")
                    {
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("🗑️ Remove Mesh Renderer", ImVec2(-1, 24)))
                        {
                            obj.removeComponent(ComponentType::MESH_RENDERER);
                            obj.meshId = -1;
                        }
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::Spacing();
            }

            // 3. RigidBody Physics Component
            if (obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS) || obj.isPhysicsEnabled)
            {
                if (ImGui::CollapsingHeader("\xe2\x9a\x96\xef\xb8\x8f RigidBody Physics Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto rb = obj.getComponent<RigidBodyComponent>();

                    // Collider Type dropdown
                        const char* colliderNames[] = { "Box", "Sphere", "Capsule", "Plane" };
                        int colliderIdx = rb ? static_cast<int>(rb->colliderType) : 0;
                        if (ImGui::Combo("Collider Type", &colliderIdx, colliderNames, IM_ARRAYSIZE(colliderNames)))
                        {
                            if (rb) rb->colliderType = static_cast<ColliderType>(colliderIdx);
                        }

                        // Motion Type dropdown
                        const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };
                        int motionIdx = rb ? static_cast<int>(rb->motionType) : 2;
                        if (ImGui::Combo("Motion Type", &motionIdx, motionNames, IM_ARRAYSIZE(motionNames)))
                        {
                            if (rb) rb->motionType = static_cast<BodyMotionType>(motionIdx);
                        }

                        ImGui::Separator();

                        // Mass
                        float mass = rb ? rb->mass : 1.0f;
                        if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.01f, 1000.0f, "%.2f kg"))
                        {
                            if (rb) rb->mass = mass;
                        }

                        // Friction
                        float friction = rb ? rb->friction : 0.5f;
                        if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f, "%.2f"))
                        {
                            if (rb) rb->friction = friction;
                        }

                        // Restitution (Bounciness)
                        float restitution = rb ? rb->restitution : 0.3f;
                        if (ImGui::SliderFloat("Restitution (Bounce)", &restitution, 0.0f, 1.0f, "%.2f"))
                        {
                            if (rb) rb->restitution = restitution;
                        }

                        ImGui::Separator();

                        // Linear Drag
                        float linearDrag = rb ? rb->linearDrag : 0.01f;
                        if (ImGui::DragFloat("Linear Drag", &linearDrag, 0.001f, 0.0f, 10.0f, "%.3f"))
                        {
                            if (rb) rb->linearDrag = linearDrag;
                        }

                        // Angular Drag
                        float angularDrag = rb ? rb->angularDrag : 0.05f;
                        if (ImGui::DragFloat("Angular Drag", &angularDrag, 0.001f, 0.0f, 10.0f, "%.3f"))
                        {
                            if (rb) rb->angularDrag = angularDrag;
                        }

                        ImGui::Separator();

                        // Gravity & Trigger checkboxes
                        bool useGravity = rb ? rb->useGravity : true;
                        if (ImGui::Checkbox("Use Gravity", &useGravity))
                        {
                            if (rb) rb->useGravity = useGravity;
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox("Enable Physics", &obj.isPhysicsEnabled);

                        bool isTrigger = rb ? rb->isTrigger : false;
                        if (ImGui::Checkbox("Is Trigger", &isTrigger))
                        {
                            if (rb) rb->isTrigger = isTrigger;
                        }

                        // Velocity display (read-only)
                        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),
                            "Velocity: (%.2f, %.2f, %.2f)", obj.velocity.x, obj.velocity.y, obj.velocity.z);

                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                        if (ImGui::Button("\xf0\x9f\x97\x91\xef\xb8\x8f Remove RigidBody Component", ImVec2(-1, 24)))
                        {
                            obj.removeComponent(ComponentType::RIGIDBODY_PHYSICS);
                            obj.isPhysicsEnabled = false;
                        }
                        ImGui::PopStyleColor();
                }
                ImGui::Spacing();
            }

            // 4. Lua Script Components (Multi-Script Support)
            if (!obj.luaScripts.empty())
            {
                int scriptToRemove = -1;
                for (int si = 0; si < static_cast<int>(obj.luaScripts.size()); si++)
                {
                    std::string headerLabel = "\xf0\x9f\x93\x96 Lua Script #" + std::to_string(si + 1) + "##lua_" + std::to_string(si);
                    if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::TextDisabled("Edit OOP script or drag a .lua file below.");
                        std::string inputId = "##LuaScript_" + std::to_string(si);
                        static char scriptBuf[8192];
                        strncpy(scriptBuf, obj.luaScripts[si].c_str(), sizeof(scriptBuf));
                        scriptBuf[sizeof(scriptBuf) - 1] = '\0';
                        if (ImGui::InputTextMultiline(inputId.c_str(), scriptBuf, sizeof(scriptBuf), ImVec2(-1.0f, 150.0f), ImGuiInputTextFlags_AllowTabInput))
                        {
                            obj.luaScripts[si] = scriptBuf;
                        }

                        // Drag-drop .lua file onto this script slot
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payloadLua = ImGui::AcceptDragDropPayload("DND_ASSET_LUA"))
                            {
                                const char* assetPath = static_cast<const char*>(payloadLua->Data);
                                std::ifstream t(assetPath);
                                if (t.is_open())
                                {
                                    std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                                    obj.luaScripts[si] = scriptContent;
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
                        std::string removeLabel = "\xf0\x9f\x97\x91\xef\xb8\x8f Remove Script #" + std::to_string(si + 1) + "##rm_lua_" + std::to_string(si);
                        if (ImGui::Button(removeLabel.c_str(), ImVec2(-1, 24)))
                        {
                            scriptToRemove = si;
                        }
                        ImGui::PopStyleColor();
                    }
                    ImGui::Spacing();
                }
                if (scriptToRemove >= 0)
                {
                    obj.luaScripts.erase(obj.luaScripts.begin() + scriptToRemove);
                    if (obj.luaScripts.empty())
                        obj.removeComponent(ComponentType::LUA_SCRIPT);
                }
            }

                        // 5. Light Component
            if (obj.hasComponent(ComponentType::LIGHT) || obj.type == ObjectType::LIGHT)
            {
                if (ImGui::CollapsingHeader("💡 Light Component", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::ColorEdit4("Light Color", &obj.color.x);
                    ImGui::Checkbox("Cast Surface Shadows", &enableShadowMapping);
                }
                ImGui::Spacing();
            }

            // --- ADD COMPONENT BUTTON & POPUP MENU ---
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.45f, 1.0f));
            if (ImGui::Button(" ➕ Add Component ", ImVec2(-1, 32)))
            {
                ImGui::OpenPopup("AddComponentPopup");
            }
            ImGui::PopStyleColor(2);

            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                ImGui::TextDisabled("-- Add New Component --");
                ImGui::Separator();

                if (!obj.hasComponent(ComponentType::MESH_RENDERER))
                {
                    if (ImGui::MenuItem("🧊 Mesh Renderer Component"))
                    {
                        auto meshComp = std::make_shared<MeshRendererComponent>();
                        meshComp->meshId = primitiveCubeMeshId;
                        obj.meshId = primitiveCubeMeshId;
                        obj.components.push_back(meshComp);
                    }
                }
                if (!obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS))
                {
                    if (ImGui::MenuItem("\xe2\x9a\x96\xef\xb8\x8f RigidBody Physics Component"))
                    {
                        auto rbComp = std::make_shared<RigidBodyComponent>();
                        // Set collider type based on object type
                        if (obj.type == ObjectType::SPHERE) rbComp->colliderType = ColliderType::SPHERE;
                        else if (obj.type == ObjectType::PLANE) rbComp->colliderType = ColliderType::PLANE;
                        else rbComp->colliderType = ColliderType::BOX;
                        rbComp->motionType = BodyMotionType::DYNAMIC;
                        obj.isPhysicsEnabled = true;
                        obj.components.push_back(rbComp);
                    }
                }
                // Always allow adding more Lua scripts (multi-script)
                {
                    if (ImGui::MenuItem("ð Add Lua Script"))
                    {
                        std::string defaultScript = "-- Lua Script #" + std::to_string(obj.luaScripts.size() + 1) + "\n";
                        defaultScript += "local Script = {}\n\n";
                        defaultScript += "function Script:onStart(obj)\n";
                        defaultScript += "    print(\"[Lua] Script started on: \" .. obj.name)\n";
                        defaultScript += "end\n\n";
                        defaultScript += "function Script:onUpdate(obj, dt)\n";
                        defaultScript += "end\n\n";
                        defaultScript += "return Script\n";
                        obj.luaScripts.push_back(defaultScript);
                        if (!obj.hasComponent(ComponentType::LUA_SCRIPT))
                        {
                            auto luaComp = std::make_shared<LuaScriptComponent>();
                            obj.components.push_back(luaComp);
                        }
                    }
                }
                if (!obj.hasComponent(ComponentType::LIGHT))
                {
                    if (ImGui::MenuItem("💡 Light Component"))
                    {
                        auto lightComp = std::make_shared<LightComponent>();
                        obj.type = ObjectType::LIGHT;
                        obj.components.push_back(lightComp);
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::Spacing();

            // Delete Entity button
            if (obj.name != "Player Cube" && obj.name != "Gold Collectible" && obj.name != "Ground Obstacle")
            {
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button(" 🗑️ Delete Entity (GameObject) ", ImVec2(-1, 30)))
                {
                    sceneObjects.erase(sceneObjects.begin() + selectedObjectIndex);
                    selectedObjectIndex = 0;
                }
                ImGui::PopStyleColor(2);
            }
        }
        else if (selectedObjectIndex == -1) // Main Camera selected
        {
            ImGui::Text("🔍 Selected Object: ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Main Camera");
            ImGui::Separator();

            ImGui::Checkbox("Static", &isStatic);
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
            ImGui::Text("Tag:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90);
            snprintf(tagBuf, sizeof(tagBuf), "MainCamera");
            ImGui::InputText("##Tag", tagBuf, sizeof(tagBuf));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Camera Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position (X,Y,Z)", &mainCameraPos.x, 0.05f);
                ImGui::DragFloat3("Target (X,Y,Z)", &mainCameraTarget.x, 0.05f);

                if (ImGui::Button("Reset Camera View"))
                {
                    mainCameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
                    mainCameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
                    mainCameraFov = 45.0f;
                }
            }

            if (ImGui::CollapsingHeader("Camera Lens Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat("Field of View (FOV)", &mainCameraFov, 10.0f, 120.0f);
                ImGui::DragFloat("Near Clip", &mainCameraNear, 0.01f, 0.01f, 5.0f);
                ImGui::DragFloat("Far Clip", &mainCameraFar, 0.5f, 5.0f, 100.0f);
            }
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();

    // 6. Project Console / Status Bar (Bottom Window)
    ImGui::SetNextWindowPos(ImVec2(0.0f, windowHeight - bottomBarHeight));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, bottomBarHeight));
    if (ImGui::Begin("Console / Project Logs", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.4f, 1.0f), "[INFO] Render Device: %s", selectedGpuName.c_str());
        if (mode == AppMode::PLAY) {
            ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.3f, 1.0f), "[GAMEPLAY] Playing! WASD or Arrow Keys to move the Player Cube. Space to jump. Collect the gold target!");
        } else {
            ImGui::TextDisabled("[CAMERA] Adjust Main Camera Pos (X,Y,Z) in Inspector or select Main Camera in Hierarchy.");
            ImGui::Text("[SCENE] Drag inside Scene window to rotate view. Scroll to zoom. Add/remove/edit objects in Hierarchy/Inspector.");
        }
        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "[SYSTEM] App mode: %s. Objects count: %d", mode == AppMode::PLAY ? "PLAY" : "EDIT", (int)sceneObjects.size());
    }
    ImGui::End();

    // 7. Visual Profiler Panel (FrameTime, CPU, GPU/VRAM, RAM)
    drawProfilerPanel();

    // 8. Asset Browser Panel & Drag-and-Drop System
    drawAssetBrowserPanel(windowWidth, bottomBarHeight);

    ImGui::Render();
}

ImVec2 VulkanApp::projectPoint(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj, const ImVec2& offset, const ImVec2& size)
{
    glm::vec4 clipSpacePos = proj * view * glm::vec4(p, 1.0f);
    if (clipSpacePos.w <= 0.0f) return ImVec2(-99999.0f, -99999.0f);

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

    float x = offset.x + (ndcSpacePos.x + 1.0f) * 0.5f * size.x;
    float y = offset.y + (1.0f - ndcSpacePos.y) * 0.5f * size.y;
    return ImVec2(x, y);
}

bool VulkanApp::getRayFromScreenPos(const ImVec2& mousePos, const ImVec2& windowPos, const ImVec2& windowSize, const glm::mat4& view, const glm::mat4& proj, glm::vec3& rayOrigin, glm::vec3& rayDir)
{
    if (windowSize.x <= 0.0f || windowSize.y <= 0.0f) return false;

    float mouseRelX = mousePos.x - windowPos.x;
    float mouseRelY = mousePos.y - windowPos.y;

    float ndcX = (mouseRelX / windowSize.x) * 2.0f - 1.0f;
    float ndcY = 1.0f - (mouseRelY / windowSize.y) * 2.0f;

    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 nearNDC(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farNDC(ndcX, ndcY, 1.0f, 1.0f);

    glm::vec4 nearWorld = invVP * nearNDC;
    if (std::abs(nearWorld.w) < 1e-6f) return false;
    nearWorld /= nearWorld.w;

    glm::vec4 farWorld = invVP * farNDC;
    if (std::abs(farWorld.w) < 1e-6f) return false;
    farWorld /= farWorld.w;

    rayOrigin = glm::vec3(nearWorld);
    rayDir = glm::normalize(glm::vec3(farWorld - nearWorld));
    return true;
}

bool VulkanApp::intersectRayPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& planePoint, const glm::vec3& planeNormal, glm::vec3& hitPoint)
{
    float denom = glm::dot(rayDir, planeNormal);
    if (std::abs(denom) < 1e-6f) return false;

    float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    if (t < 0.0f) return false;

    hitPoint = rayOrigin + t * rayDir;
    return true;
}

float VulkanApp::getClosestPointOnAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& pivotPos, const glm::vec3& axisDir)
{
    glm::vec3 a = glm::normalize(axisDir);
    glm::vec3 camDir = mainCameraPos - pivotPos;
    if (glm::length(camDir) < 0.001f) camDir = glm::vec3(0.0f, 0.0f, 1.0f);
    else camDir = glm::normalize(camDir);

    // Construct a plane normal containing axis 'a' that faces towards the camera
    glm::vec3 side = glm::cross(a, camDir);
    if (glm::length(side) < 0.001f)
    {
        side = glm::cross(a, glm::vec3(0.0f, 1.0f, 0.0f));
        if (glm::length(side) < 0.001f)
            side = glm::cross(a, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    side = glm::normalize(side);
    glm::vec3 planeNormal = glm::normalize(glm::cross(side, a));

    // Intersect mouse ray with plane passing through pivotPos
    glm::vec3 hitPoint;
    if (intersectRayPlane(rayOrigin, rayDir, pivotPos, planeNormal, hitPoint))
    {
        return glm::dot(hitPoint - pivotPos, a);
    }
    return 0.0f;
}

void VulkanApp::drawSceneView(const ImVec2& windowPos, const ImVec2& windowSize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Draw background
    drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), IM_COL32(40, 40, 40, 255));

    // Calculate Camera Matrices
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), windowSize.x / windowSize.y, 0.1f, 100.0f);
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -sceneCameraDistance));
    view = glm::rotate(view, glm::radians(sceneRotationX), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, glm::radians(sceneRotationY), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraWorldPos = glm::vec3(invView[3]);
    glm::vec3 cameraForward = glm::normalize(glm::vec3(-invView[2]));

    // Tool Overlay & Active Drag Status Banner
    bool hasSelection = (selectedObjectIndex >= -1 && selectedObjectIndex < static_cast<int>(sceneObjects.size()));
    float toolWidth = (gizmoDragState.isDragging && hasSelection) ? 460.0f : 360.0f;
    drawList->AddRectFilled(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + toolWidth, windowPos.y + 35), IM_COL32(20, 20, 25, 230), 4.0f);
    drawList->AddRect(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + toolWidth, windowPos.y + 35), gizmoDragState.isDragging ? IM_COL32(255, 200, 50, 255) : IM_COL32(100, 100, 100, 255), 4.0f);

    char toolStr[256];
    const char* toolNames[] = { "HAND", "TRANSLATE", "ROTATE", "SCALE", "RECT", "COMBINED" };
    if (gizmoDragState.isDragging && hasSelection)
    {
        std::string objName = (selectedObjectIndex == -1) ? "Main Camera" : sceneObjects[selectedObjectIndex].name;
        const char* axisNames[] = { "NONE", "X Axis", "Y Axis", "Z Axis", "XY Plane", "YZ Plane", "XZ Plane", "Free Plane" };
        snprintf(toolStr, sizeof(toolStr), "Tool: %s | 📦 Dragging: %s [%s]", toolNames[static_cast<int>(activeGizmo)], objName.c_str(), axisNames[static_cast<int>(gizmoDragState.axis)]);
    }
    else
    {
        snprintf(toolStr, sizeof(toolStr), "Tool: %s  [Q/W/E/R/T/Y]", toolNames[static_cast<int>(activeGizmo)]);
    }
    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 15), IM_COL32(255, 255, 255, 255), toolStr);

    // Get selected object's 3D position
    glm::vec3 pivotPos(0.0f);
    if (hasSelection)
    {
        pivotPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
    }

    // Project selected object center and axis handles
    ImVec2 sP(-99999.0f, -99999.0f);
    ImVec2 sX(-99999.0f, -99999.0f);
    ImVec2 sY(-99999.0f, -99999.0f);
    ImVec2 sZ(-99999.0f, -99999.0f);

    float L = 0.15f * sceneCameraDistance;
    if (hasSelection)
    {
        sP = projectPoint(pivotPos, view, proj, windowPos, windowSize);
        sX = projectPoint(pivotPos + glm::vec3(L, 0.0f, 0.0f), view, proj, windowPos, windowSize);
        sY = projectPoint(pivotPos + glm::vec3(0.0f, L, 0.0f), view, proj, windowPos, windowSize);
        sZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, L), view, proj, windowPos, windowSize);
    }

    ImVec2 mousePos = ImGui::GetMousePos();
    bool isMouseInWindow = ImGui::IsWindowHovered();

    // 1. ACTIVE DRAGGING EXECUTION
    if (gizmoDragState.isDragging)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && hasSelection)
        {
            glm::vec3 curRayOrig, curRayDir;
            if (getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, curRayOrig, curRayDir))
            {
                glm::vec3 dummyRot(0.0f), dummyScale(1.0f);
                glm::vec3& posRef = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
                glm::vec3& rotRef = (selectedObjectIndex == -1) ? dummyRot : sceneObjects[selectedObjectIndex].rotation;
                glm::vec3& scaleRef = (selectedObjectIndex == -1) ? dummyScale : sceneObjects[selectedObjectIndex].scale;

                if (gizmoDragState.gizmoType == GizmoType::TRANSLATE || gizmoDragState.gizmoType == GizmoType::TRANSFORM_COMBINED)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Y || gizmoDragState.axis == DragAxis::Z)
                    {
                        float curAxisVal = getClosestPointOnAxis(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.axisDir);
                        float delta = curAxisVal - gizmoDragState.startAxisVal;
                        posRef = gizmoDragState.startObjPos + gizmoDragState.axisDir * delta;
                    }
                    else if (gizmoDragState.axis == DragAxis::FREE)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.planeNormal, curHit))
                        {
                            glm::vec3 delta = curHit - gizmoDragState.startHitPoint;
                            posRef = gizmoDragState.startObjPos + delta;
                        }
                    }
                }
                else if (gizmoDragState.gizmoType == GizmoType::ROTATE)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Y || gizmoDragState.axis == DragAxis::Z)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.planeNormal, curHit))
                        {
                            glm::vec3 dirVec = curHit - gizmoDragState.pivotPos;
                            if (glm::length(dirVec) > 0.001f)
                            {
                                float curAngle = atan2(glm::dot(dirVec, gizmoDragState.rotBasisV), glm::dot(dirVec, gizmoDragState.rotBasisU));
                                float deltaAngle = curAngle - gizmoDragState.startAngle;
                                while (deltaAngle > glm::pi<float>()) deltaAngle -= glm::two_pi<float>();
                                while (deltaAngle < -glm::pi<float>()) deltaAngle += glm::two_pi<float>();

                                rotRef = gizmoDragState.startObjRot + gizmoDragState.axisDir * glm::degrees(deltaAngle);
                            }
                        }
                    }
                }
                else if (gizmoDragState.gizmoType == GizmoType::SCALE)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Y || gizmoDragState.axis == DragAxis::Z)
                    {
                        float curAxisVal = getClosestPointOnAxis(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.axisDir);
                        float delta = curAxisVal - gizmoDragState.startAxisVal;
                        int idx = (gizmoDragState.axis == DragAxis::X) ? 0 : (gizmoDragState.axis == DragAxis::Y) ? 1 : 2;
                        scaleRef = gizmoDragState.startObjScale;
                        scaleRef[idx] = glm::max(0.01f, gizmoDragState.startObjScale[idx] + delta / L);
                    }
                    else if (gizmoDragState.axis == DragAxis::FREE)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.planeNormal, curHit))
                        {
                            float curDist = glm::distance(curHit, gizmoDragState.pivotPos);
                            float scaleFactor = (gizmoDragState.startDist > 0.001f) ? (curDist / gizmoDragState.startDist) : 1.0f;
                            scaleRef = glm::max(glm::vec3(0.01f), gizmoDragState.startObjScale * scaleFactor);
                        }
                    }
                }
                else if (gizmoDragState.gizmoType == GizmoType::RECT)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Z)
                    {
                        float curAxisVal = getClosestPointOnAxis(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.axisDir);
                        float delta = curAxisVal - gizmoDragState.startAxisVal;
                        int idx = (gizmoDragState.axis == DragAxis::X) ? 0 : 2;
                        scaleRef = gizmoDragState.startObjScale;
                        scaleRef[idx] = glm::max(0.01f, gizmoDragState.startObjScale[idx] + delta / L);
                    }
                }

                // Snap support when Ctrl is held down
                if (ImGui::GetIO().KeyCtrl && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    auto& obj = sceneObjects[selectedObjectIndex];
                    if (activeGizmo == GizmoType::TRANSLATE) {
                        obj.position.x = std::round(obj.position.x * 2.0f) / 2.0f;
                        obj.position.y = std::round(obj.position.y * 2.0f) / 2.0f;
                        obj.position.z = std::round(obj.position.z * 2.0f) / 2.0f;
                    } else if (activeGizmo == GizmoType::ROTATE) {
                        obj.rotation.x = std::round(obj.rotation.x / 15.0f) * 15.0f;
                        obj.rotation.y = std::round(obj.rotation.y / 15.0f) * 15.0f;
                        obj.rotation.z = std::round(obj.rotation.z / 15.0f) * 15.0f;
                    } else if (activeGizmo == GizmoType::SCALE || activeGizmo == GizmoType::RECT) {
                        obj.scale.x = std::round(obj.scale.x / 0.1f) * 0.1f;
                        obj.scale.y = std::round(obj.scale.y / 0.1f) * 0.1f;
                        obj.scale.z = std::round(obj.scale.z / 0.1f) * 0.1f;
                    }
                }

                // Lock boundaries for special game objects
                if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    if (sceneObjects[selectedObjectIndex].name == "Ground Obstacle")
                        sceneObjects[selectedObjectIndex].position.y = -1.5f;
                    if (sceneObjects[selectedObjectIndex].name == "Gold Collectible")
                        sceneObjects[selectedObjectIndex].position.y = -1.2f;
                }

                // Re-update pivotPos and project handle positions so Gizmo stays 100% attached to object while dragging
                if (hasSelection)
                {
                    pivotPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
                    sP = projectPoint(pivotPos, view, proj, windowPos, windowSize);
                    sX = projectPoint(pivotPos + glm::vec3(L, 0.0f, 0.0f), view, proj, windowPos, windowSize);
                    sY = projectPoint(pivotPos + glm::vec3(0.0f, L, 0.0f), view, proj, windowPos, windowSize);
                    sZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, L), view, proj, windowPos, windowSize);
                }
            }
        }
        else
        {
            // Release mouse
            gizmoDragState.isDragging = false;
            gizmoDragState.axis = DragAxis::NONE;
            activeDragAxis = DragAxis::NONE;
            isDraggingObject = false;
        }
    }

    // 2. HOVER DETECTION AND DRAG INITIALIZATION
    hoveredDragAxis = DragAxis::NONE;
    if (hasSelection && sP.x > -90000.0f && activeGizmo != GizmoType::HAND && !gizmoDragState.isDragging)
    {
        auto distToSeg = [](glm::vec2 p, glm::vec2 a, glm::vec2 b) -> float {
            glm::vec2 ab = b - a;
            float len2 = glm::dot(ab, ab);
            if (len2 < 0.001f) return glm::distance(p, a);
            float t = glm::clamp(glm::dot(p - a, ab) / len2, 0.0f, 1.0f);
            glm::vec2 proj = a + t * ab;
            return glm::distance(p, proj);
        };

        glm::vec2 mPos(mousePos.x, mousePos.y);
        float distCenter = glm::distance(mPos, glm::vec2(sP.x, sP.y));
        float distX = (sX.x > -90000.0f) ? distToSeg(mPos, glm::vec2(sP.x, sP.y), glm::vec2(sX.x, sX.y)) : 99999.0f;
        float distY = (sY.x > -90000.0f) ? distToSeg(mPos, glm::vec2(sP.x, sP.y), glm::vec2(sY.x, sY.y)) : 99999.0f;
        float distZ = (sZ.x > -90000.0f) ? distToSeg(mPos, glm::vec2(sP.x, sP.y), glm::vec2(sZ.x, sZ.y)) : 99999.0f;

        if (activeGizmo == GizmoType::TRANSLATE || activeGizmo == GizmoType::TRANSFORM_COMBINED || activeGizmo == GizmoType::SCALE)
        {
            if (distCenter < 14.0f) hoveredDragAxis = DragAxis::FREE;
            else if (distX < 12.0f) hoveredDragAxis = DragAxis::X;
            else if (distY < 12.0f) hoveredDragAxis = DragAxis::Y;
            else if (distZ < 12.0f) hoveredDragAxis = DragAxis::Z;
        }
        else if (activeGizmo == GizmoType::ROTATE)
        {
            float bestRingDist = 12.0f;
            const int numSegs = 36;
            for (int i = 0; i < numSegs; ++i)
            {
                float a1 = (i * 2.0f * 3.14159f) / numSegs;
                float a2 = ((i + 1) * 2.0f * 3.14159f) / numSegs;

                // X ring (YZ plane)
                ImVec2 rx1 = projectPoint(pivotPos + glm::vec3(0.0f, cos(a1) * L, sin(a1) * L), view, proj, windowPos, windowSize);
                ImVec2 rx2 = projectPoint(pivotPos + glm::vec3(0.0f, cos(a2) * L, sin(a2) * L), view, proj, windowPos, windowSize);
                if (rx1.x > -90000.0f && rx2.x > -90000.0f) {
                    float d = distToSeg(mPos, glm::vec2(rx1.x, rx1.y), glm::vec2(rx2.x, rx2.y));
                    if (d < bestRingDist) { bestRingDist = d; hoveredDragAxis = DragAxis::X; }
                }

                // Y ring (XZ plane)
                ImVec2 ry1 = projectPoint(pivotPos + glm::vec3(cos(a1) * L, 0.0f, sin(a1) * L), view, proj, windowPos, windowSize);
                ImVec2 ry2 = projectPoint(pivotPos + glm::vec3(cos(a2) * L, 0.0f, sin(a2) * L), view, proj, windowPos, windowSize);
                if (ry1.x > -90000.0f && ry2.x > -90000.0f) {
                    float d = distToSeg(mPos, glm::vec2(ry1.x, ry1.y), glm::vec2(ry2.x, ry2.y));
                    if (d < bestRingDist) { bestRingDist = d; hoveredDragAxis = DragAxis::Y; }
                }

                // Z ring (XY plane)
                ImVec2 rz1 = projectPoint(pivotPos + glm::vec3(cos(a1) * L, sin(a1) * L, 0.0f), view, proj, windowPos, windowSize);
                ImVec2 rz2 = projectPoint(pivotPos + glm::vec3(cos(a2) * L, sin(a2) * L, 0.0f), view, proj, windowPos, windowSize);
                if (rz1.x > -90000.0f && rz2.x > -90000.0f) {
                    float d = distToSeg(mPos, glm::vec2(rz1.x, rz1.y), glm::vec2(rz2.x, rz2.y));
                    if (d < bestRingDist) { bestRingDist = d; hoveredDragAxis = DragAxis::Z; }
                }
            }
        }
        else if (activeGizmo == GizmoType::RECT)
        {
            if (distX < 12.0f) hoveredDragAxis = DragAxis::X;
            else if (distZ < 12.0f) hoveredDragAxis = DragAxis::Z;
        }
    }

    if (hoveredDragAxis != DragAxis::NONE || gizmoDragState.isDragging)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    // Handle mouse click to start drag or select object
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && isMouseInWindow && !gizmoDragState.isDragging)
    {
        if (hoveredDragAxis != DragAxis::NONE && hasSelection)
        {
            saveHistory();
            gizmoDragState.isDragging = true;
            gizmoDragState.axis = hoveredDragAxis;
            gizmoDragState.gizmoType = activeGizmo;
            activeDragAxis = hoveredDragAxis;
            isDraggingObject = true;

            gizmoDragState.startObjPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
            gizmoDragState.startObjRot = (selectedObjectIndex == -1) ? glm::vec3(0.0f) : sceneObjects[selectedObjectIndex].rotation;
            gizmoDragState.startObjScale = (selectedObjectIndex == -1) ? glm::vec3(1.0f) : sceneObjects[selectedObjectIndex].scale;
            gizmoDragState.pivotPos = pivotPos;

            glm::vec3 rayOrig, rayDir;
            getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, rayOrig, rayDir);

            if (activeGizmo == GizmoType::TRANSLATE || activeGizmo == GizmoType::TRANSFORM_COMBINED || activeGizmo == GizmoType::SCALE || activeGizmo == GizmoType::RECT)
            {
                if (hoveredDragAxis == DragAxis::X)
                {
                    gizmoDragState.axisDir = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.startAxisVal = getClosestPointOnAxis(rayOrig, rayDir, pivotPos, gizmoDragState.axisDir);
                }
                else if (hoveredDragAxis == DragAxis::Y)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.startAxisVal = getClosestPointOnAxis(rayOrig, rayDir, pivotPos, gizmoDragState.axisDir);
                }
                else if (hoveredDragAxis == DragAxis::Z)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 0.0f, 1.0f);
                    gizmoDragState.startAxisVal = getClosestPointOnAxis(rayOrig, rayDir, pivotPos, gizmoDragState.axisDir);
                }
                else if (hoveredDragAxis == DragAxis::FREE)
                {
                    gizmoDragState.planeNormal = cameraForward;
                    intersectRayPlane(rayOrig, rayDir, pivotPos, gizmoDragState.planeNormal, gizmoDragState.startHitPoint);
                    gizmoDragState.startDist = glm::distance(gizmoDragState.startHitPoint, pivotPos);
                }
            }
            else if (activeGizmo == GizmoType::ROTATE)
            {
                if (hoveredDragAxis == DragAxis::X)
                {
                    gizmoDragState.axisDir = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.planeNormal = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.rotBasisU = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.rotBasisV = glm::vec3(0.0f, 0.0f, 1.0f);
                }
                else if (hoveredDragAxis == DragAxis::Y)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.rotBasisU = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.rotBasisV = glm::vec3(0.0f, 0.0f, 1.0f);
                }
                else if (hoveredDragAxis == DragAxis::Z)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 0.0f, 1.0f);
                    gizmoDragState.planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                    gizmoDragState.rotBasisU = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.rotBasisV = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                intersectRayPlane(rayOrig, rayDir, pivotPos, gizmoDragState.planeNormal, gizmoDragState.startHitPoint);
                glm::vec3 dirVec = gizmoDragState.startHitPoint - pivotPos;
                gizmoDragState.startAngle = atan2(glm::dot(dirVec, gizmoDragState.rotBasisV), glm::dot(dirVec, gizmoDragState.rotBasisU));
            }
        }
        else
        {
            // Click to select center object
            float minDistance = 22.0f;
            int closestIdx = -2;

            ImVec2 camScreenPos = projectPoint(mainCameraPos, view, proj, windowPos, windowSize);
            if (camScreenPos.x > -90000.0f)
            {
                float dist = glm::distance(glm::vec2(mousePos.x, mousePos.y), glm::vec2(camScreenPos.x, camScreenPos.y));
                if (dist < minDistance) { minDistance = dist; closestIdx = -1; }
            }

            for (size_t i = 0; i < sceneObjects.size(); ++i)
            {
                ImVec2 screenPos = projectPoint(sceneObjects[i].position, view, proj, windowPos, windowSize);
                if (screenPos.x > -90000.0f)
                {
                    float dist = glm::distance(glm::vec2(mousePos.x, mousePos.y), glm::vec2(screenPos.x, screenPos.y));
                    if (dist < minDistance) { minDistance = dist; closestIdx = static_cast<int>(i); }
                }
            }

            if (closestIdx != -2)
            {
                selectedObjectIndex = closestIdx;
            }
        }
    }

    // Camera Navigation Orbiting (Hand tool or dragging background)
    if (activeGizmo == GizmoType::HAND || (!gizmoDragState.isDragging && hoveredDragAxis == DragAxis::NONE && ImGui::IsMouseDown(ImGuiMouseButton_Left) && isMouseInWindow))
    {
        ImGui::SetCursorScreenPos(windowPos);
        ImGui::InvisibleButton("##SceneDragArea", windowSize);
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            sceneRotationY += delta.x * 0.5f;
            sceneRotationX += delta.y * 0.5f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
    }

    if (ImGui::IsItemHovered())
    {
        sceneCameraDistance -= ImGui::GetIO().MouseWheel * 0.5f;
        sceneCameraDistance = std::clamp(sceneCameraDistance, 2.0f, 20.0f);
    }

    // 1. Draw Grid lines in XZ plane (y = -1.5)
    float gridY = -1.5f;
    for (int i = -5; i <= 5; ++i)
    {
        // Parallel to Z
        glm::vec3 p1(i, gridY, -5.0f);
        glm::vec3 p2(i, gridY, 5.0f);
        ImVec2 sp1 = projectPoint(p1, view, proj, windowPos, windowSize);
        ImVec2 sp2 = projectPoint(p2, view, proj, windowPos, windowSize);
        if (sp1.x > -90000.0f && sp2.x > -90000.0f)
        {
            ImU32 col = (i == 0) ? IM_COL32(0, 0, 180, 255) : IM_COL32(80, 80, 80, 255);
            drawList->AddLine(sp1, sp2, col, (i == 0) ? 2.0f : 1.0f);
        }

        // Parallel to X
        glm::vec3 p3(-5.0f, gridY, i);
        glm::vec3 p4(5.0f, gridY, i);
        ImVec2 sp3 = projectPoint(p3, view, proj, windowPos, windowSize);
        ImVec2 sp4 = projectPoint(p4, view, proj, windowPos, windowSize);
        if (sp3.x > -90000.0f && sp4.x > -90000.0f)
        {
            ImU32 col = (i == 0) ? IM_COL32(180, 0, 0, 255) : IM_COL32(80, 80, 80, 255);
            drawList->AddLine(sp3, sp4, col, (i == 0) ? 2.0f : 1.0f);
        }
    }

    // 2. Draw 3D Origin Axes
    glm::vec3 orig(0.0f, gridY, 0.0f);
    glm::vec3 axX(1.0f, gridY, 0.0f);
    glm::vec3 axY(0.0f, gridY + 1.0f, 0.0f);
    glm::vec3 axZ(0.0f, gridY, 1.0f);
    ImVec2 sor = projectPoint(orig, view, proj, windowPos, windowSize);
    ImVec2 sx = projectPoint(axX, view, proj, windowPos, windowSize);
    ImVec2 sy = projectPoint(axY, view, proj, windowPos, windowSize);
    ImVec2 sz = projectPoint(axZ, view, proj, windowPos, windowSize);
    if (sor.x > -90000.0f)
    {
        if (sx.x > -90000.0f) {
            drawList->AddLine(sor, sx, IM_COL32(255, 0, 0, 255), 2.0f);
            drawList->AddText(ImVec2(sx.x + 4.0f, sx.y - 4.0f), IM_COL32(255, 100, 100, 255), "X");
        }
        if (sy.x > -90000.0f) {
            drawList->AddLine(sor, sy, IM_COL32(0, 255, 0, 255), 2.0f);
            drawList->AddText(ImVec2(sy.x + 4.0f, sy.y - 4.0f), IM_COL32(100, 255, 100, 255), "Y");
        }
        if (sz.x > -90000.0f) {
            drawList->AddLine(sor, sz, IM_COL32(0, 0, 255, 255), 2.0f);
            drawList->AddText(ImVec2(sz.x + 4.0f, sz.y - 4.0f), IM_COL32(100, 100, 255, 255), "Z");
        }
    }

    // 3. Draw All Scene Objects in 3D Offscreen View
    ImGui::SetCursorScreenPos(windowPos);
    if (offscreenDescriptorSet) {
        ImGui::Image((ImTextureID)offscreenDescriptorSet, windowSize);
    }

    // 4. Draw Gizmos & Selection Highlights
    DragAxis activeHighlight = gizmoDragState.isDragging ? gizmoDragState.axis : hoveredDragAxis;
    if (hasSelection && sP.x > -90000.0f && activeGizmo != GizmoType::HAND)
    {
        // Pivot Center indicator
        ImU32 centerCol = (activeHighlight == DragAxis::FREE) ? IM_COL32(255, 255, 100, 255) : IM_COL32(255, 255, 0, 255);
        drawList->AddCircle(sP, (activeHighlight == DragAxis::FREE) ? 13.0f : 11.0f, centerCol, 0, 2.0f);
        drawList->AddLine(ImVec2(sP.x - 7, sP.y), ImVec2(sP.x + 7, sP.y), centerCol, 1.5f);
        drawList->AddLine(ImVec2(sP.x, sP.y - 7), ImVec2(sP.x, sP.y + 7), centerCol, 1.5f);

        if (activeGizmo == GizmoType::TRANSLATE || activeGizmo == GizmoType::TRANSFORM_COMBINED)
        {
            // X-Axis (Red)
            if (sX.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::X) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::X) ? IM_COL32(255, 130, 130, 255) : IM_COL32(255, 40, 40, 255);
                drawList->AddLine(sP, sX, col, thick);
                drawList->AddRectFilled(ImVec2(sX.x - 5, sX.y - 5), ImVec2(sX.x + 5, sX.y + 5), col);
            }

            // Y-Axis (Green)
            if (sY.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Y) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Y) ? IM_COL32(130, 255, 130, 255) : IM_COL32(40, 255, 40, 255);
                drawList->AddLine(sP, sY, col, thick);
                drawList->AddRectFilled(ImVec2(sY.x - 5, sY.y - 5), ImVec2(sY.x + 5, sY.y + 5), col);
            }

            // Z-Axis (Blue)
            if (sZ.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Z) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Z) ? IM_COL32(130, 130, 255, 255) : IM_COL32(40, 40, 255, 255);
                drawList->AddLine(sP, sZ, col, thick);
                drawList->AddRectFilled(ImVec2(sZ.x - 5, sZ.y - 5), ImVec2(sZ.x + 5, sZ.y + 5), col);
            }
        }

        if (activeGizmo == GizmoType::ROTATE || activeGizmo == GizmoType::TRANSFORM_COMBINED)
        {
            const int numSegs = 36;
            ImVec2 prevX, prevY, prevZ;
            for (int i = 0; i <= numSegs; ++i)
            {
                float a = (i * 2.0f * 3.14159f) / numSegs;

                // X-Ring (Red - YZ plane)
                glm::vec3 ptX = pivotPos + glm::vec3(0.0f, cos(a) * L, sin(a) * L);
                ImVec2 sPtX = projectPoint(ptX, view, proj, windowPos, windowSize);
                if (i > 0 && prevX.x > -90000.0f && sPtX.x > -90000.0f)
                {
                    bool isH = (activeHighlight == DragAxis::X);
                    ImU32 col = isH ? IM_COL32(255, 140, 140, 255) : IM_COL32(255, 60, 60, 200);
                    drawList->AddLine(prevX, sPtX, col, isH ? 3.5f : 1.8f);
                }
                prevX = sPtX;

                // Y-Ring (Green - XZ plane)
                glm::vec3 ptY = pivotPos + glm::vec3(cos(a) * L, 0.0f, sin(a) * L);
                ImVec2 sPtY = projectPoint(ptY, view, proj, windowPos, windowSize);
                if (i > 0 && prevY.x > -90000.0f && sPtY.x > -90000.0f)
                {
                    bool isH = (activeHighlight == DragAxis::Y);
                    ImU32 col = isH ? IM_COL32(140, 255, 140, 255) : IM_COL32(60, 255, 60, 200);
                    drawList->AddLine(prevY, sPtY, col, isH ? 3.5f : 1.8f);
                }
                prevY = sPtY;

                // Z-Ring (Blue - XY plane)
                glm::vec3 ptZ = pivotPos + glm::vec3(cos(a) * L, sin(a) * L, 0.0f);
                ImVec2 sPtZ = projectPoint(ptZ, view, proj, windowPos, windowSize);
                if (i > 0 && prevZ.x > -90000.0f && sPtZ.x > -90000.0f)
                {
                    bool isH = (activeHighlight == DragAxis::Z);
                    ImU32 col = isH ? IM_COL32(140, 140, 255, 255) : IM_COL32(60, 60, 255, 200);
                    drawList->AddLine(prevZ, sPtZ, col, isH ? 3.5f : 1.8f);
                }
                prevZ = sPtZ;
            }

            if (activeGizmo == GizmoType::ROTATE)
            {
                if (sX.x > -90000.0f) drawList->AddCircleFilled(sX, 5.0f, (activeHighlight == DragAxis::X) ? IM_COL32(255, 160, 160, 255) : IM_COL32(255, 0, 0, 255));
                if (sY.x > -90000.0f) drawList->AddCircleFilled(sY, 5.0f, (activeHighlight == DragAxis::Y) ? IM_COL32(160, 255, 160, 255) : IM_COL32(0, 255, 0, 255));
                if (sZ.x > -90000.0f) drawList->AddCircleFilled(sZ, 5.0f, (activeHighlight == DragAxis::Z) ? IM_COL32(160, 160, 255, 255) : IM_COL32(0, 0, 255, 255));
            }
        }

        if (activeGizmo == GizmoType::SCALE)
        {
            if (sX.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::X) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::X) ? IM_COL32(255, 130, 130, 255) : IM_COL32(255, 40, 40, 255);
                drawList->AddLine(sP, sX, col, thick);
                drawList->AddRectFilled(ImVec2(sX.x - 5, sX.y - 5), ImVec2(sX.x + 5, sX.y + 5), col);
            }
            if (sY.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Y) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Y) ? IM_COL32(130, 255, 130, 255) : IM_COL32(40, 255, 40, 255);
                drawList->AddLine(sP, sY, col, thick);
                drawList->AddRectFilled(ImVec2(sY.x - 5, sY.y - 5), ImVec2(sY.x + 5, sY.y + 5), col);
            }
            if (sZ.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Z) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Z) ? IM_COL32(130, 130, 255, 255) : IM_COL32(40, 40, 255, 255);
                drawList->AddLine(sP, sZ, col, thick);
                drawList->AddRectFilled(ImVec2(sZ.x - 5, sZ.y - 5), ImVec2(sZ.x + 5, sZ.y + 5), col);
            }
        }

        if (activeGizmo == GizmoType::RECT)
        {
            glm::vec3 halfX(L * 0.8f, 0.0f, 0.0f);
            glm::vec3 halfZ(0.0f, 0.0f, L * 0.8f);

            ImVec2 tl = projectPoint(pivotPos - halfX + halfZ, view, proj, windowPos, windowSize);
            ImVec2 tr = projectPoint(pivotPos + halfX + halfZ, view, proj, windowPos, windowSize);
            ImVec2 br = projectPoint(pivotPos + halfX - halfZ, view, proj, windowPos, windowSize);
            ImVec2 bl = projectPoint(pivotPos - halfX - halfZ, view, proj, windowPos, windowSize);

            if (tl.x > -90000.0f && tr.x > -90000.0f && br.x > -90000.0f && bl.x > -90000.0f)
            {
                ImU32 rectColor = IM_COL32(200, 200, 200, 150);
                drawList->AddQuad(tl, tr, br, bl, rectColor, 1.5f);

                if (sX.x > -90000.0f) drawList->AddCircleFilled(sX, 4.0f, IM_COL32(255, 0, 0, 255));
                if (sZ.x > -90000.0f) drawList->AddCircleFilled(sZ, 4.0f, IM_COL32(0, 0, 255, 255));
            }
        }
    }

    // 5. Floating Active Drag Tooltip & Status Badge
    if (gizmoDragState.isDragging && hasSelection)
    {
        std::string objName = (selectedObjectIndex == -1) ? "Main Camera" : sceneObjects[selectedObjectIndex].name;
        const char* axisNames[] = { "NONE", "X Axis", "Y Axis", "Z Axis", "XY Plane", "YZ Plane", "XZ Plane", "Free" };
        const char* axisStr = axisNames[static_cast<int>(gizmoDragState.axis)];

        ImU32 axisBadgeCol = IM_COL32(255, 255, 100, 255);
        if (gizmoDragState.axis == DragAxis::X) axisBadgeCol = IM_COL32(255, 80, 80, 255);
        else if (gizmoDragState.axis == DragAxis::Y) axisBadgeCol = IM_COL32(80, 255, 80, 255);
        else if (gizmoDragState.axis == DragAxis::Z) axisBadgeCol = IM_COL32(80, 150, 255, 255);

        glm::vec3 curPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
        glm::vec3 curRot = (selectedObjectIndex == -1) ? glm::vec3(0.0f) : sceneObjects[selectedObjectIndex].rotation;
        glm::vec3 curScale = (selectedObjectIndex == -1) ? glm::vec3(1.0f) : sceneObjects[selectedObjectIndex].scale;

        char badgeLine1[128];
        char badgeLine2[128];
        char badgeLine3[128] = "";

        if (gizmoDragState.gizmoType == GizmoType::TRANSLATE || gizmoDragState.gizmoType == GizmoType::TRANSFORM_COMBINED)
        {
            snprintf(badgeLine1, sizeof(badgeLine1), "🎯 Moving: %s [%s]", objName.c_str(), axisStr);
            if (gizmoDragState.axis == DragAxis::X)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos X: %.2fm  (ΔX: %+.2fm)", curPos.x, curPos.x - gizmoDragState.startObjPos.x);
            else if (gizmoDragState.axis == DragAxis::Y)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos Y: %.2fm  (ΔY: %+.2fm)", curPos.y, curPos.y - gizmoDragState.startObjPos.y);
            else if (gizmoDragState.axis == DragAxis::Z)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos Z: %.2fm  (ΔZ: %+.2fm)", curPos.z, curPos.z - gizmoDragState.startObjPos.z);
            else
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos: (%.2f, %.2f, %.2f)", curPos.x, curPos.y, curPos.z);
        }
        else if (gizmoDragState.gizmoType == GizmoType::ROTATE)
        {
            snprintf(badgeLine1, sizeof(badgeLine1), "🔄 Rotating: %s [%s]", objName.c_str(), axisStr);
            if (gizmoDragState.axis == DragAxis::X)
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot X: %.1f°  (Δ: %+.1f°)", curRot.x, curRot.x - gizmoDragState.startObjRot.x);
            else if (gizmoDragState.axis == DragAxis::Y)
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot Y: %.1f°  (Δ: %+.1f°)", curRot.y, curRot.y - gizmoDragState.startObjRot.y);
            else if (gizmoDragState.axis == DragAxis::Z)
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot Z: %.1f°  (Δ: %+.1f°)", curRot.z, curRot.z - gizmoDragState.startObjRot.z);
            else
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot: (%.1f°, %.1f°, %.1f°)", curRot.x, curRot.y, curRot.z);
        }
        else if (gizmoDragState.gizmoType == GizmoType::SCALE || gizmoDragState.gizmoType == GizmoType::RECT)
        {
            snprintf(badgeLine1, sizeof(badgeLine1), "📐 Scaling: %s [%s]", objName.c_str(), axisStr);
            if (gizmoDragState.axis == DragAxis::X)
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale X: %.2f  (Ratio: %.2fx)", curScale.x, (gizmoDragState.startObjScale.x > 0.001f) ? curScale.x / gizmoDragState.startObjScale.x : 1.0f);
            else if (gizmoDragState.axis == DragAxis::Y)
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale Y: %.2f  (Ratio: %.2fx)", curScale.y, (gizmoDragState.startObjScale.y > 0.001f) ? curScale.y / gizmoDragState.startObjScale.y : 1.0f);
            else if (gizmoDragState.axis == DragAxis::Z)
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale Z: %.2f  (Ratio: %.2fx)", curScale.z, (gizmoDragState.startObjScale.z > 0.001f) ? curScale.z / gizmoDragState.startObjScale.z : 1.0f);
            else
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale: (%.2f, %.2f, %.2f)", curScale.x, curScale.y, curScale.z);
        }

        if (ImGui::GetIO().KeyCtrl)
        {
            snprintf(badgeLine3, sizeof(badgeLine3), "🧲 SNAP GRID ACTIVE");
        }

        // Draw Floating Badge near mouse cursor
        ImVec2 badgePos = ImVec2(mousePos.x + 20.0f, mousePos.y + 15.0f);
        float badgeWidth = 250.0f;
        float badgeHeight = (badgeLine3[0] != '\0') ? 66.0f : 48.0f;

        if (badgePos.x + badgeWidth > windowPos.x + windowSize.x - 10.0f)
            badgePos.x = mousePos.x - badgeWidth - 10.0f;
        if (badgePos.y + badgeHeight > windowPos.y + windowSize.y - 10.0f)
            badgePos.y = mousePos.y - badgeHeight - 10.0f;

        drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + badgeWidth, badgePos.y + badgeHeight), IM_COL32(15, 18, 24, 235), 6.0f);
        drawList->AddRect(badgePos, ImVec2(badgePos.x + badgeWidth, badgePos.y + badgeHeight), axisBadgeCol, 6.0f, 0, 1.8f);

        drawList->AddText(ImVec2(badgePos.x + 10.0f, badgePos.y + 6.0f), IM_COL32(255, 255, 255, 255), badgeLine1);
        drawList->AddText(ImVec2(badgePos.x + 10.0f, badgePos.y + 24.0f), IM_COL32(200, 220, 255, 255), badgeLine2);
        if (badgeLine3[0] != '\0')
        {
            drawList->AddText(ImVec2(badgePos.x + 10.0f, badgePos.y + 44.0f), IM_COL32(100, 255, 255, 255), badgeLine3);
        }
    }

    // 5. Draw 3D Camera Gizmo and frustum
    glm::vec3 camPos = mainCameraPos;
    glm::vec3 camTarget = mainCameraTarget;
    glm::vec3 forwardVector = glm::normalize(camTarget - camPos);
    glm::vec3 rightVector = glm::cross(forwardVector, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(rightVector) < 0.01f) rightVector = glm::vec3(1.0f, 0.0f, 0.0f);
    else rightVector = glm::normalize(rightVector);
    glm::vec3 upVector = glm::cross(rightVector, forwardVector);

    float frustumDist = 0.5f;
    float frustumW = 0.25f;
    float frustumH = 0.18f;

    glm::vec3 cameraBaseCenter = camPos + forwardVector * frustumDist;
    glm::vec3 c0 = cameraBaseCenter - rightVector * frustumW + upVector * frustumH;
    glm::vec3 c1 = cameraBaseCenter + rightVector * frustumW + upVector * frustumH;
    glm::vec3 c2 = cameraBaseCenter + rightVector * frustumW - upVector * frustumH;
    glm::vec3 c3 = cameraBaseCenter - rightVector * frustumW - upVector * frustumH;

    ImVec2 scamPos = projectPoint(camPos, view, proj, windowPos, windowSize);
    ImVec2 sc0 = projectPoint(c0, view, proj, windowPos, windowSize);
    ImVec2 sc1 = projectPoint(c1, view, proj, windowPos, windowSize);
    ImVec2 sc2 = projectPoint(c2, view, proj, windowPos, windowSize);
    ImVec2 sc3 = projectPoint(c3, view, proj, windowPos, windowSize);

    ImU32 frustumColor = IM_COL32(0, 240, 255, 255);
    
    if (scamPos.x > -90000.0f)
    {
        // Draw frustum lines
        if (sc0.x > -90000.0f) drawList->AddLine(scamPos, sc0, frustumColor, 1.5f);
        if (sc1.x > -90000.0f) drawList->AddLine(scamPos, sc1, frustumColor, 1.5f);
        if (sc2.x > -90000.0f) drawList->AddLine(scamPos, sc2, frustumColor, 1.5f);
        if (sc3.x > -90000.0f) drawList->AddLine(scamPos, sc3, frustumColor, 1.5f);

        // Draw lens borders
        if (sc0.x > -90000.0f && sc1.x > -90000.0f) drawList->AddLine(sc0, sc1, frustumColor, 1.5f);
        if (sc1.x > -90000.0f && sc2.x > -90000.0f) drawList->AddLine(sc1, sc2, frustumColor, 1.5f);
        if (sc2.x > -90000.0f && sc3.x > -90000.0f) drawList->AddLine(sc2, sc3, frustumColor, 1.5f);
        if (sc3.x > -90000.0f && sc0.x > -90000.0f) drawList->AddLine(sc3, sc0, frustumColor, 1.5f);

        // Draw camera body behind apex
        glm::vec3 bodyBack = camPos - forwardVector * 0.15f;
        glm::vec3 b0 = bodyBack - rightVector * 0.12f + upVector * 0.09f;
        glm::vec3 b1 = bodyBack + rightVector * 0.12f + upVector * 0.09f;
        glm::vec3 b2 = bodyBack + rightVector * 0.12f - upVector * 0.09f;
        glm::vec3 b3 = bodyBack - rightVector * 0.12f - upVector * 0.09f;

        ImVec2 sb0 = projectPoint(b0, view, proj, windowPos, windowSize);
        ImVec2 sb1 = projectPoint(b1, view, proj, windowPos, windowSize);
        ImVec2 sb2 = projectPoint(b2, view, proj, windowPos, windowSize);
        ImVec2 sb3 = projectPoint(b3, view, proj, windowPos, windowSize);

        if (sb0.x > -90000.0f && sb1.x > -90000.0f && sb2.x > -90000.0f && sb3.x > -90000.0f)
        {
            drawList->AddLine(sb0, sb1, frustumColor, 1.0f);
            drawList->AddLine(sb1, sb2, frustumColor, 1.0f);
            drawList->AddLine(sb2, sb3, frustumColor, 1.0f);
            drawList->AddLine(sb3, sb0, frustumColor, 1.0f);

            drawList->AddLine(scamPos, sb0, frustumColor, 1.0f);
            drawList->AddLine(scamPos, sb1, frustumColor, 1.0f);
            drawList->AddLine(scamPos, sb2, frustumColor, 1.0f);
            drawList->AddLine(scamPos, sb3, frustumColor, 1.0f);
        }

        // Draw camera icon (filled circle) & label
        drawList->AddCircleFilled(scamPos, 7.0f, IM_COL32(0, 240, 255, 255));
        drawList->AddCircle(scamPos, 11.0f, IM_COL32(0, 240, 255, 120), 0, 1.5f);
        drawList->AddText(ImVec2(scamPos.x + 12.0f, scamPos.y - 7.0f), IM_COL32(0, 240, 255, 255), "🎥 Main Camera");

        // Target line
        ImVec2 sorPoint = projectPoint(camTarget, view, proj, windowPos, windowSize);
        if (sorPoint.x > -90000.0f)
        {
            drawList->AddLine(scamPos, sorPoint, IM_COL32(0, 240, 255, 100), 1.0f);
        }
    }

    // 6. Drag & Drop Target over Scene View (Raycast to 3D Drop Position)
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payloadModel = ImGui::AcceptDragDropPayload("DND_ASSET_MODEL");
        const ImGuiPayload* payloadTex = ImGui::AcceptDragDropPayload("DND_ASSET_TEXTURE");
        const ImGuiPayload* payloadLua = ImGui::AcceptDragDropPayload("DND_ASSET_LUA");
        const ImGuiPayload* payloadGeneric = ImGui::AcceptDragDropPayload("DND_ASSET_PATH");

        const ImGuiPayload* payload = payloadModel ? payloadModel : (payloadTex ? payloadTex : (payloadLua ? payloadLua : payloadGeneric));

        if (payload)
        {
            const char* assetPath = static_cast<const char*>(payload->Data);
            std::string pathStr(assetPath);

            ImVec2 mousePos = ImGui::GetMousePos();
            glm::vec3 rayOrig, rayDir;
            glm::vec3 dropPos(0.0f, 0.0f, 0.0f);
            if (getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, rayOrig, rayDir))
            {
                glm::vec3 hitPoint;
                if (intersectRayPlane(rayOrig, rayDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), hitPoint))
                {
                    dropPos = hitPoint;
                }
            }

            std::filesystem::path p(pathStr);
            std::string ext = p.extension().string();
            for (auto& c : ext) c = tolower(c);

            saveHistory();

            if (pathStr == "PRIMITIVE_CUBE")
            {
                SceneObject newObj;
                newObj.id = sceneObjects.size();
                newObj.name = "Cube " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::CUBE;
                newObj.position = dropPos;
                newObj.scale = glm::vec3(0.5f);
                newObj.color = glm::vec4(1.0f);
                newObj.meshId = primitiveCubeMeshId;
                sceneObjects.push_back(newObj);
                selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
            }
            else if (pathStr == "PRIMITIVE_SPHERE")
            {
                SceneObject newObj;
                newObj.id = sceneObjects.size();
                newObj.name = "Sphere " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::SPHERE;
                newObj.position = dropPos;
                newObj.scale = glm::vec3(0.5f);
                newObj.color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
                newObj.meshId = primitiveSphereMeshId;
                sceneObjects.push_back(newObj);
                selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
            }
            else if (pathStr == "PRIMITIVE_PLANE")
            {
                SceneObject newObj;
                newObj.id = sceneObjects.size();
                newObj.name = "Plane " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::PLANE;
                newObj.position = dropPos;
                newObj.scale = glm::vec3(2.0f, 1.0f, 2.0f);
                newObj.color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                newObj.meshId = primitivePlaneMeshId;
                sceneObjects.push_back(newObj);
                selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
            }
            else if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
            {
                int meshId = load3DModelAsset(pathStr);
                if (meshId >= 0)
                {
                    SceneObject newObj;
                    newObj.id = sceneObjects.size();
                    newObj.name = p.stem().string();
                    newObj.type = ObjectType::CUBE;
                    newObj.position = dropPos;
                    newObj.scale = glm::vec3(1.0f);
                    newObj.color = glm::vec4(1.0f);
                    newObj.meshId = meshId;
                    sceneObjects.push_back(newObj);
                    selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
                }
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
            {
                int texId = loadTextureAsset(pathStr);
                if (texId >= 0 && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    sceneObjects[selectedObjectIndex].textureId = texId;
                }
            }
            else if (ext == ".lua")
            {
                if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    std::ifstream t(pathStr);
                    if (t.is_open())
                    {
                        std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                        sceneObjects[selectedObjectIndex].luaScripts.push_back(scriptContent);
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}
void VulkanApp::drawGameView(const ImVec2& windowPos, const ImVec2& windowSize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 1. Draw Space Gradient Background
    drawList->AddRectFilledMultiColor(
        windowPos, 
        ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
        IM_COL32(6, 10, 20, 255),   // Top-Left
        IM_COL32(8, 12, 24, 255),   // Top-Right
        IM_COL32(16, 26, 46, 255),  // Bottom-Right
        IM_COL32(10, 18, 32, 255)   // Bottom-Left
    );

    // 2. Draw Planetary Nebula Glow
    drawList->AddCircleFilled(ImVec2(windowPos.x + windowSize.x * 0.75f, windowPos.y + windowSize.y * 0.70f), 130.0f, IM_COL32(0, 120, 255, 12), 64);
    drawList->AddCircleFilled(ImVec2(windowPos.x + windowSize.x * 0.75f, windowPos.y + windowSize.y * 0.70f), 80.0f, IM_COL32(100, 180, 255, 18), 64);
    drawList->AddCircleFilled(ImVec2(windowPos.x + windowSize.x * 0.25f, windowPos.y + windowSize.y * 0.30f), 200.0f, IM_COL32(120, 80, 255, 8), 64);

    // 3. Draw Stars
    for (int i = 0; i < 40; ++i)
    {
        float sx = static_cast<float>((i * 13579) % static_cast<int>(windowSize.x - 20) + 10);
        float sy = static_cast<float>((i * 24680) % static_cast<int>(windowSize.y - 20) + 10);
        float radius = (i % 4 == 0) ? 1.5f : (i % 7 == 0 ? 2.0f : 1.0f);
        ImU32 starCol = (i % 6 == 0) ? IM_COL32(200, 230, 255, 230) : IM_COL32(255, 255, 255, 160);
        if (i % 12 == 0) {
            drawList->AddLine(ImVec2(windowPos.x + sx - 3, windowPos.y + sy), ImVec2(windowPos.x + sx + 3, windowPos.y + sy), IM_COL32(255, 255, 255, 150));
            drawList->AddLine(ImVec2(windowPos.x + sx, windowPos.y + sy - 3), ImVec2(windowPos.x + sx, windowPos.y + sy + 3), IM_COL32(255, 255, 255, 150));
        }
        drawList->AddCircleFilled(ImVec2(windowPos.x + sx, windowPos.y + sy), radius, starCol);
    }

    // Camera Matrices for Game view rendering (POV)
    glm::mat4 gameProj = glm::perspective(glm::radians(mainCameraFov), windowSize.x / windowSize.y, mainCameraNear, mainCameraFar);
    glm::mat4 gameView = glm::lookAt(mainCameraPos, mainCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    // 4. Draw Ground Platform in Game View (represented by XZ grid lines)
    float gridY = -1.5f;
    for (int i = -4; i <= 4; ++i)
    {
        glm::vec3 p1(i, gridY, -4.0f);
        glm::vec3 p2(i, gridY, 4.0f);
        ImVec2 sp1 = projectPoint(p1, gameView, gameProj, windowPos, windowSize);
        ImVec2 sp2 = projectPoint(p2, gameView, gameProj, windowPos, windowSize);
        if (sp1.x > -90000.0f && sp2.x > -90000.0f)
            drawList->AddLine(sp1, sp2, IM_COL32(50, 80, 100, 180), 1.0f);

        glm::vec3 p3(-4.0f, gridY, i);
        glm::vec3 p4(4.0f, gridY, i);
        ImVec2 sp3 = projectPoint(p3, gameView, gameProj, windowPos, windowSize);
        ImVec2 sp4 = projectPoint(p4, gameView, gameProj, windowPos, windowSize);
        if (sp3.x > -90000.0f && sp4.x > -90000.0f)
            drawList->AddLine(sp3, sp4, IM_COL32(50, 80, 100, 180), 1.0f);
    }

    // 5. Draw all 3D objects in the Game View (from Main Camera perspective)
    ImGui::SetCursorScreenPos(windowPos);
    if (gameViewDescriptorSet) {
        ImGui::Image((ImTextureID)gameViewDescriptorSet, windowSize);
    }

    // 6. Draw Game HUD (Score Overlay)
    drawList->AddRectFilled(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + 220, windowPos.y + 85), IM_COL32(15, 20, 30, 200), 4.0f);
    drawList->AddRect(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + 220, windowPos.y + 85), IM_COL32(0, 180, 255, 150), 4.0f);

    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 15), IM_COL32(0, 240, 255, 255), "ANTIGRAVITY mini-game");
    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", gameScore);
    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 35), IM_COL32(50, 255, 100, 255), scoreText);
    char hiText[64];
    snprintf(hiText, sizeof(hiText), "High Score: %d", highScore);
    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 50), IM_COL32(255, 215, 0, 255), hiText);
    
    if (mode == AppMode::PLAY)
    {
        drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 65), IM_COL32(200, 200, 200, 255), "Keys: WASD + Space (Jump)");
    }
    else
    {
        drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 65), IM_COL32(255, 150, 0, 255), "EDIT MODE: Click PLAY above");
    }
}

glm::mat4 VulkanApp::getWorldMatrix(const std::vector<SceneObject>& objects, int index) const
{
    if (index < 0 || index >= objects.size()) return glm::mat4(1.0f);
    
    glm::mat4 localMat = glm::mat4(1.0f);
    localMat = glm::translate(localMat, objects[index].position);
    localMat = glm::rotate(localMat, glm::radians(objects[index].rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    localMat = glm::rotate(localMat, glm::radians(objects[index].rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    localMat = glm::rotate(localMat, glm::radians(objects[index].rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    localMat = glm::scale(localMat, objects[index].scale);
    
    if (objects[index].parentId != -1)
    {
        return getWorldMatrix(objects, objects[index].parentId) * localMat;
    }
    return localMat;
}

void VulkanApp::saveHistory()
{
    undoStack.push_back(sceneObjects);
    redoStack.clear();
}

void VulkanApp::undo()
{
    if (!undoStack.empty())
    {
        redoStack.push_back(sceneObjects);
        sceneObjects = undoStack.back();
        undoStack.pop_back();
    }
}

void VulkanApp::redo()
{
    if (!redoStack.empty())
    {
        undoStack.push_back(sceneObjects);
        sceneObjects = redoStack.back();
        redoStack.pop_back();
    }
}

void VulkanApp::initializeDefaultScene()
{
    sceneObjects.clear();
    gameScore = 0;
    
    // 1. Player Cube
    SceneObject player;
    player.name = "Player Cube";
    player.type = ObjectType::CUBE;
    player.position = glm::vec3(0.0f, 0.0f, 0.0f);
    player.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    player.scale = glm::vec3(0.5f, 0.5f, 0.5f);
    player.color = glm::vec4(0.0f, 0.8f, 1.0f, 1.0f);
    player.isPhysicsEnabled = true;
    player.velocity = glm::vec3(0.0f);
    player.id = 0;
    player.meshId = primitiveCubeMeshId;     // <-- assign mesh!
    player.bodyData = physEngine.createBody(ColliderType::BOX, player.position, player.scale, BodyMotionType::DYNAMIC, 1.0f, 0.5f, 0.3f);
    sceneObjects.push_back(player);
    
    playerStartPos = player.position;

    // 2. Collectible Sphere
    SceneObject target;
    target.id = 1;
    target.name = "Gold Collectible";
    target.type = ObjectType::SPHERE;
    target.position = glm::vec3(1.5f, -1.0f, 1.0f);
    target.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    target.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    target.color = glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);
    target.isPhysicsEnabled = false;
    target.meshId = primitiveSphereMeshId;   // <-- assign mesh!
    target.bodyData = physEngine.createBody(ColliderType::SPHERE, target.position, target.scale, BodyMotionType::DYNAMIC, 0.5f, 0.5f, 0.5f);
    sceneObjects.push_back(target);

    // 3. Ground Obstacle Plane
    SceneObject ground;
    ground.id = 2;
    ground.name = "Ground Obstacle";
    ground.type = ObjectType::PLANE;
    ground.position = glm::vec3(0.0f, -1.5f, 0.0f);
    ground.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    ground.scale = glm::vec3(5.0f, 0.1f, 5.0f);
    ground.color = glm::vec4(0.3f, 0.3f, 0.35f, 1.0f);
    ground.isPhysicsEnabled = false;
    ground.meshId = primitivePlaneMeshId;    // <-- assign mesh!
    ground.bodyData = physEngine.createBody(ColliderType::PLANE, ground.position, ground.scale, BodyMotionType::STATIC, 0.0f, 0.8f, 0.1f);
    sceneObjects.push_back(ground);

    // 4. Directional Light (represented as a small cube gizmo)
    SceneObject sun;
    sun.id = 3;
    sun.name = "Directional Light";
    sun.type = ObjectType::LIGHT;
    sun.position = glm::vec3(2.0f, 3.0f, 1.0f);
    sun.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
    sun.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    sun.color = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);
    sun.isPhysicsEnabled = false;
    sun.meshId = primitiveCubeMeshId;        // <-- assign mesh (small cube gizmo)
    sceneObjects.push_back(sun);
    
    selectedObjectIndex = 0;
    saveHistory(); // Save initial state
}


void VulkanApp::updatePhysics(float deltaTime)
{
    // Make sure deltaTime is reasonable
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    // 0. Update ALL Lua OOP Scripts (multi-script per object)
    for (auto& obj : sceneObjects)
    {
        for (auto& luaInst : obj.luaInstances)
        {
            if (luaInst.valid())
            {
                sol::protected_function onUpdate = luaInst["onUpdate"];
                if (onUpdate.valid())
                {
                    auto res = onUpdate(luaInst, &obj, deltaTime);
                    if (!res.valid())
                    {
                        sol::error err = res;
                        printf("Lua onUpdate error: %s\n", err.what());
                    }
                }
            }
        }
    }

    // 1. Step the Jolt Physics World
    physEngine.update(deltaTime);

    // 2. Sync physics positions/rotations back to all scene objects
    syncPhysicsToTransform();

    // 3. Player-specific gameplay logic (WASD, jump, camera follow, score)
    SceneObject* player = nullptr;
    for (auto& obj : sceneObjects)
    {
        if (obj.name == "Player Cube")
        {
            player = &obj;
            break;
        }
    }

    if (player)
    {
        // Keyboard control inputs (WASD & Arrow Keys)
        float speed = 3.0f;
        glm::vec3 moveDir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            moveDir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            moveDir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            moveDir.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            moveDir.x += 1.0f;

        if (glm::length(moveDir) > 0.0f)
        {
            moveDir = glm::normalize(moveDir);
            if (player->bodyData)
            {
                // Apply movement as force through Jolt
                auto rb = player->getComponent<RigidBodyComponent>();
                float forceMag = speed * (rb ? rb->mass : 1.0f) * 10.0f;
                physEngine.addForce(player->bodyData, moveDir * forceMag);
            }
            else
            {
                player->position.x += moveDir.x * speed * deltaTime;
                player->position.z += moveDir.z * speed * deltaTime;
            }
        }

        // Jump (allow only when grounded)
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            float groundY = -1.5f;
            float halfHeight = player->scale.y * 0.5f;
            if (player->position.y - halfHeight <= groundY + 0.1f)
            {
                if (player->bodyData)
                {
                    physEngine.addImpulse(player->bodyData, glm::vec3(0.0f, 5.0f, 0.0f));
                }
                else
                {
                    player->velocity.y = 5.0f;
                }
            }
        }

        // Fallback manual physics if no Jolt body
        if (!player->bodyData)
        {
            player->velocity.y -= 9.81f * deltaTime;
            player->position.y += player->velocity.y * deltaTime;
            float groundY = -1.5f;
            float halfHeight = player->scale.y * 0.5f;
            if (player->position.y - halfHeight < groundY)
            {
                player->position.y = groundY + halfHeight;
                player->velocity.y = -player->velocity.y * 0.5f;
                if (glm::abs(player->velocity.y) < 0.1f) player->velocity.y = 0.0f;
            }
        }

        // Grid boundaries constraint
        player->position.x = std::clamp(player->position.x, -5.0f, 5.0f);
        player->position.z = std::clamp(player->position.z, -5.0f, 5.0f);

        // Write position back to Jolt body
        if (player->bodyData)
        {
            physEngine.setBodyPosition(player->bodyData, player->position);
        }

        // Update main camera target to follow player cube
        mainCameraTarget = player->position;
        mainCameraPos = player->position + glm::vec3(2.5f, 2.5f, 2.5f);

        // Collision detection with Collectible target
        SceneObject* target = nullptr;
        for (auto& obj : sceneObjects)
        {
            if (obj.name == "Gold Collectible")
            {
                target = &obj;
                break;
            }
        }

        if (target)
        {
            float dist = glm::distance(player->position, target->position);
            float limit = (player->scale.x + target->scale.x) * 0.5f;
            if (dist < limit)
            {
                // Increment score and relocate target
                gameScore++;
                if (gameScore > highScore) highScore = gameScore;

                float rx = -3.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 6.0f));
                float rz = -3.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 6.0f));
                target->position = glm::vec3(rx, -1.2f, rz);
            }
        }
    }
}

// ---- Physics Engine Helper Methods ----

void VulkanApp::initPhysicsBodies()
{
    for (auto& obj : sceneObjects)
    {
        // Remove old body if exists
        if (obj.bodyData)
        {
            physEngine.removeBody(obj.bodyData);
            obj.bodyData = nullptr;
        }

        if (obj.isPhysicsEnabled || obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS))
        {
            auto rb = obj.getComponent<RigidBodyComponent>();
            ColliderType ct = ColliderType::BOX;
            BodyMotionType mt = BodyMotionType::DYNAMIC;
            float mass = 1.0f;
            float friction = 0.5f;
            float restitution = 0.3f;

            if (rb)
            {
                ct = rb->colliderType;
                mt = rb->motionType;
                mass = rb->mass;
                friction = rb->friction;
                restitution = rb->restitution;
            }
            else
            {
                // Determine collider from ObjectType
                if (obj.type == ObjectType::SPHERE) ct = ColliderType::SPHERE;
                else if (obj.type == ObjectType::PLANE) ct = ColliderType::PLANE;
            }

            obj.bodyData = physEngine.createBody(ct, obj.position, obj.scale, mt, mass, friction, restitution);
        }
    }
}

void VulkanApp::syncPhysicsToTransform()
{
    for (auto& obj : sceneObjects)
    {
        if (!obj.bodyData) continue;

        auto rb = obj.getComponent<RigidBodyComponent>();
        if (!rb) continue;
        if (rb->motionType == BodyMotionType::STATIC) continue;

        // Skip player cube - gameplay code handles its position
        if (obj.name == "Player Cube") continue;

        // Read position/rotation from Jolt Physics
        obj.position = physEngine.getBodyPosition(obj.bodyData);
        glm::quat q = physEngine.getBodyRotation(obj.bodyData);
        obj.rotation = glm::degrees(glm::eulerAngles(q));

        // Sync velocity for display
        obj.velocity = physEngine.getLinearVelocity(obj.bodyData);

        // Apply linear drag
        if (rb->linearDrag > 0.0f)
        {
            glm::vec3 vel = obj.velocity;
            vel *= (1.0f - rb->linearDrag);
            physEngine.setLinearVelocity(obj.bodyData, vel);
        }

        // Apply angular drag
        if (rb->angularDrag > 0.0f)
        {
            glm::vec3 avel = physEngine.getAngularVelocity(obj.bodyData);
            avel *= (1.0f - rb->angularDrag);
            physEngine.setAngularVelocity(obj.bodyData, avel);
        }
    }
}

void VulkanApp::savePlayModeState()
{
    for (auto& obj : sceneObjects)
    {
        obj.savedPosition = obj.position;
        obj.savedRotation = obj.rotation;
        obj.savedScale = obj.scale;
    }
    playerStartPos = glm::vec3(0.0f);
    for (auto& obj : sceneObjects)
    {
        if (obj.name == "Player Cube")
        {
            playerStartPos = obj.position;
            break;
        }
    }
}

void VulkanApp::restoreEditModeState()
{
    for (auto& obj : sceneObjects)
    {
        obj.position = obj.savedPosition;
        obj.rotation = obj.savedRotation;
        obj.scale = obj.savedScale;
        obj.velocity = glm::vec3(0.0f);

        // Destroy Jolt physics bodies
        if (obj.bodyData)
        {
            physEngine.removeBody(obj.bodyData);
            obj.bodyData = nullptr;
        }
    }
}

void VulkanApp::draw3DObject(int objIndex, const glm::mat4& view, const glm::mat4& proj, const ImVec2& offset, const ImVec2& size)
{
    const SceneObject& obj = sceneObjects[objIndex];
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 col = IM_COL32(obj.color.r * 255, obj.color.g * 255, obj.color.b * 255, obj.color.a * 255);

    // Build Model Matrix
    glm::mat4 model = getWorldMatrix(sceneObjects, objIndex);

    if (obj.type == ObjectType::CUBE)
    {
        // 8 Corners of a Cube
        glm::vec3 localV[8] = {
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f,  0.5f}
        };
        ImVec2 projV[8];
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 worldPos = glm::vec3(model * glm::vec4(localV[i], 1.0f));
            projV[i] = projectPoint(worldPos, view, proj, offset, size);
        }

        auto drawEdge = [&](int idx1, int idx2) {
            if (projV[idx1].x > -90000.0f && projV[idx2].x > -90000.0f)
                drawList->AddLine(projV[idx1], projV[idx2], col, 1.5f);
        };

        // Front Face
        drawEdge(0, 1); drawEdge(1, 2); drawEdge(2, 3); drawEdge(3, 0);
        // Back Face
        drawEdge(4, 5); drawEdge(5, 6); drawEdge(6, 7); drawEdge(7, 4);
        // Side connectors
        drawEdge(0, 4); drawEdge(1, 5); drawEdge(2, 6); drawEdge(3, 7);
    }
    else if (obj.type == ObjectType::SPHERE)
    {
        // Draw 3 Orthogonal circles
        const int numSegments = 16;
        ImVec2 prevXZ, prevXY, prevYZ;
        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = (i * 2.0f * 3.14159f) / numSegments;
            
            // XZ Ring
            glm::vec3 ptXZ(cos(angle) * 0.5f, 0.0f, sin(angle) * 0.5f);
            glm::vec3 wXZ = glm::vec3(model * glm::vec4(ptXZ, 1.0f));
            ImVec2 pXZ = projectPoint(wXZ, view, proj, offset, size);
            if (i > 0 && prevXZ.x > -90000.0f && pXZ.x > -90000.0f)
                drawList->AddLine(prevXZ, pXZ, col, 1.5f);
            prevXZ = pXZ;

            // XY Ring
            glm::vec3 ptXY(cos(angle) * 0.5f, sin(angle) * 0.5f, 0.0f);
            glm::vec3 wXY = glm::vec3(model * glm::vec4(ptXY, 1.0f));
            ImVec2 pXY = projectPoint(wXY, view, proj, offset, size);
            if (i > 0 && prevXY.x > -90000.0f && pXY.x > -90000.0f)
                drawList->AddLine(prevXY, pXY, col, 1.5f);
            prevXY = pXY;
        }
    }
    else if (obj.type == ObjectType::PLANE)
    {
        // 4 Corners in local space
        glm::vec3 localP[4] = {
            {-0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, 0.5f}, {-0.5f, 0.0f, 0.5f}
        };
        ImVec2 projP[4];
        for (int i = 0; i < 4; ++i)
        {
            glm::vec3 worldPos = glm::vec3(model * glm::vec4(localP[i], 1.0f));
            projP[i] = projectPoint(worldPos, view, proj, offset, size);
        }

        if (projP[0].x > -90000.0f && projP[1].x > -90000.0f && projP[2].x > -90000.0f && projP[3].x > -90000.0f)
        {
            drawList->AddQuadFilled(projP[0], projP[1], projP[2], projP[3], IM_COL32(obj.color.r * 255, obj.color.g * 255, obj.color.b * 255, 35));
            drawList->AddQuad(projP[0], projP[1], projP[2], projP[3], col, 1.5f);
        }
    }
    else if (obj.type == ObjectType::LIGHT)
    {
        ImVec2 center = projectPoint(obj.position, view, proj, offset, size);
        if (center.x > -90000.0f)
        {
            // Light symbol (bulb symbol)
            drawList->AddCircleFilled(center, 4.0f, IM_COL32(255, 255, 100, 255));
            drawList->AddCircle(center, 8.0f, col, 0, 1.5f);
            
            // Calculate actual light direction from rotation
            glm::mat4 rotM = glm::mat4(1.0f);
            rotM = glm::rotate(rotM, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
            rotM = glm::rotate(rotM, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            rotM = glm::rotate(rotM, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            glm::vec3 lookDir = glm::normalize(glm::vec3(rotM * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
            glm::vec3 right = glm::normalize(glm::vec3(rotM * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
            glm::vec3 up = glm::normalize(glm::vec3(rotM * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

            // 1. Draw Camera-like frustum for the light
            float frustumDist = 0.6f;
            float frustumW = 0.35f;
            float frustumH = 0.35f;

            glm::vec3 baseCenter = obj.position + lookDir * frustumDist;
            glm::vec3 c0 = baseCenter - right * frustumW + up * frustumH;
            glm::vec3 c1 = baseCenter + right * frustumW + up * frustumH;
            glm::vec3 c2 = baseCenter + right * frustumW - up * frustumH;
            glm::vec3 c3 = baseCenter - right * frustumW - up * frustumH;

            ImVec2 sc0 = projectPoint(c0, view, proj, offset, size);
            ImVec2 sc1 = projectPoint(c1, view, proj, offset, size);
            ImVec2 sc2 = projectPoint(c2, view, proj, offset, size);
            ImVec2 sc3 = projectPoint(c3, view, proj, offset, size);

            ImU32 frustumCol = IM_COL32(255, 255, 100, 255);
            if (sc0.x > -90000.0f) drawList->AddLine(center, sc0, frustumCol, 1.5f);
            if (sc1.x > -90000.0f) drawList->AddLine(center, sc1, frustumCol, 1.5f);
            if (sc2.x > -90000.0f) drawList->AddLine(center, sc2, frustumCol, 1.5f);
            if (sc3.x > -90000.0f) drawList->AddLine(center, sc3, frustumCol, 1.5f);

            if (sc0.x > -90000.0f && sc1.x > -90000.0f) drawList->AddLine(sc0, sc1, frustumCol, 1.5f);
            if (sc1.x > -90000.0f && sc2.x > -90000.0f) drawList->AddLine(sc1, sc2, frustumCol, 1.5f);
            if (sc2.x > -90000.0f && sc3.x > -90000.0f) drawList->AddLine(sc2, sc3, frustumCol, 1.5f);
            if (sc3.x > -90000.0f && sc0.x > -90000.0f) drawList->AddLine(sc3, sc0, frustumCol, 1.5f);
            
            // 2. Draw main ray to the ground (y = 0)
            float t = (lookDir.y < -0.001f) ? (-obj.position.y / lookDir.y) : 10.0f;
            if (t < 0.0f || t > 50.0f) t = 10.0f;
            
            glm::vec3 endPt = obj.position + lookDir * t;
            ImVec2 end = projectPoint(endPt, view, proj, offset, size);
            
            if (end.x > -90000.0f)
            {
                // Main light direction ray
                drawList->AddLine(center, end, IM_COL32(255, 255, 100, 150), 1.0f);
                
                // Draw a cross on the plane where the light hits
                glm::vec3 p1 = endPt + glm::vec3(1, 0, 0);
                glm::vec3 p2 = endPt + glm::vec3(-1, 0, 0);
                glm::vec3 p3 = endPt + glm::vec3(0, 0, 1);
                glm::vec3 p4 = endPt + glm::vec3(0, 0, -1);
                ImVec2 sp1 = projectPoint(p1, view, proj, offset, size);
                ImVec2 sp2 = projectPoint(p2, view, proj, offset, size);
                ImVec2 sp3 = projectPoint(p3, view, proj, offset, size);
                ImVec2 sp4 = projectPoint(p4, view, proj, offset, size);
                if (sp1.x > -90000.0f && sp2.x > -90000.0f) drawList->AddLine(sp1, sp2, IM_COL32(255, 150, 0, 150), 1.5f);
                if (sp3.x > -90000.0f && sp4.x > -90000.0f) drawList->AddLine(sp3, sp4, IM_COL32(255, 150, 0, 150), 1.5f);
            }
        }
    }
}

void VulkanApp::saveScene(const std::string& filename)
{
    std::ofstream out(filename);
    if (!out.is_open()) return;

    out << "# ShapeRenderer Scene File\n";
    for (const auto& obj : sceneObjects)
    {
        out << "object: " << obj.name << "\n";
        out << "type: " << static_cast<int>(obj.type) << "\n";
        out << "position: " << obj.position.x << " " << obj.position.y << " " << obj.position.z << "\n";
        out << "rotation: " << obj.rotation.x << " " << obj.rotation.y << " " << obj.rotation.z << "\n";
        out << "scale: " << obj.scale.x << " " << obj.scale.y << " " << obj.scale.z << "\n";
        out << "color: " << obj.color.r << " " << obj.color.g << " " << obj.color.b << " " << obj.color.a << "\n";
        out << "physics: " << (obj.isPhysicsEnabled ? 1 : 0) << "\n";
        // Save physics properties
        auto rbSave = obj.getComponent<RigidBodyComponent>();
        if (rbSave)
        {
            out << "mass: " << rbSave->mass << "\n";
            out << "friction: " << rbSave->friction << "\n";
            out << "restitution: " << rbSave->restitution << "\n";
            out << "linearDrag: " << rbSave->linearDrag << "\n";
            out << "angularDrag: " << rbSave->angularDrag << "\n";
            out << "colliderType: " << static_cast<int>(rbSave->colliderType) << "\n";
            out << "motionType: " << static_cast<int>(rbSave->motionType) << "\n";
            out << "useGravity: " << (rbSave->useGravity ? 1 : 0) << "\n";
        }
        
        // Save Lua scripts
        out << "luaScriptsCount: " << obj.luaScripts.size() << "\n";
        for (const auto& script : obj.luaScripts)
        {
            out << "luaScriptStart:\n";
            out << script;
            if (!script.empty() && script.back() != '\n') out << "\n";
            out << "luaScriptEnd:\n";
        }
        out << "\n";
    }
    out.close();
}

void VulkanApp::loadScene(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in.is_open()) return;

    sceneObjects.clear();
    std::string line;
    SceneObject current;
    bool hasObj = false;

    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "object:")
        {
            if (hasObj)
            {
                sceneObjects.push_back(current);
            }
            std::string name;
            std::getline(ss, name);
            if (!name.empty() && name[0] == ' ') name = name.substr(1);
            current = SceneObject();
            current.name = name;
            hasObj = true;
        }
        else if (key == "type:")
        {
            int t;
            ss >> t;
            current.type = static_cast<ObjectType>(t);
        }
        else if (key == "position:")
        {
            ss >> current.position.x >> current.position.y >> current.position.z;
        }
        else if (key == "rotation:")
        {
            ss >> current.rotation.x >> current.rotation.y >> current.rotation.z;
        }
        else if (key == "scale:")
        {
            ss >> current.scale.x >> current.scale.y >> current.scale.z;
        }
        else if (key == "color:")
        {
            ss >> current.color.r >> current.color.g >> current.color.b >> current.color.a;
        }
        else if (key == "physics:")
        {
            int p;
            ss >> p;
            current.isPhysicsEnabled = (p == 1);
            if (current.isPhysicsEnabled)
            {
                if (!current.hasComponent(ComponentType::RIGIDBODY_PHYSICS))
                {
                    auto rb = std::make_shared<RigidBodyComponent>();
                    current.components.push_back(rb);
                }
            }
        }
        else if (key == "mass:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->mass = v;
        }
        else if (key == "friction:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->friction = v;
        }
        else if (key == "restitution:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->restitution = v;
        }
        else if (key == "linearDrag:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->linearDrag = v;
        }
        else if (key == "angularDrag:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->angularDrag = v;
        }
        else if (key == "colliderType:")
        {
            int v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->colliderType = static_cast<ColliderType>(v);
        }
        else if (key == "motionType:")
        {
            int v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->motionType = static_cast<BodyMotionType>(v);
        }
        else if (key == "useGravity:")
        {
            int v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->useGravity = (v == 1);
        }
        else if (key == "luaScriptStart:")
        {
            std::string scriptContent = "";
            std::string scriptLine;
            while (std::getline(in, scriptLine))
            {
                if (!scriptLine.empty() && scriptLine.back() == '\r') scriptLine.pop_back();
                if (scriptLine == "luaScriptEnd:") break;
                scriptContent += scriptLine + "\n";
            }
            if (!scriptContent.empty()) scriptContent.pop_back();
            current.luaScripts.push_back(scriptContent);
        }
    }
    if (hasObj)
    {
        sceneObjects.push_back(current);
    }
    in.close();

    // Reset starting position for player cube if loaded
    for (auto& obj : sceneObjects)
    {
        if (obj.name == "Player Cube")
        {
            playerStartPos = obj.position;
        }
        // Assign mesh based on object type (meshId is not saved/loaded)
        switch (obj.type) {
            case ObjectType::CUBE:  obj.meshId = primitiveCubeMeshId;   break;
            case ObjectType::SPHERE: obj.meshId = primitiveSphereMeshId; break;
            case ObjectType::PLANE: obj.meshId = primitivePlaneMeshId;  break;
            case ObjectType::LIGHT: obj.meshId = primitiveCubeMeshId;   break;
            default: obj.meshId = -1; break;
        }
    }
    selectedObjectIndex = 0;
}

void VulkanApp::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void VulkanApp::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanApp::updateUniformBuffer(uint32_t currentImage)
{
    UniformBufferObject ubo{};
    
    // Scene Camera UBO
    glm::vec3 camPos(
        sceneCameraDistance * cos(glm::radians(sceneRotationX)) * sin(glm::radians(sceneRotationY)),
        sceneCameraDistance * sin(glm::radians(sceneRotationX)),
        sceneCameraDistance * cos(glm::radians(sceneRotationX)) * cos(glm::radians(sceneRotationY))
    );
    ubo.view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;
    // Find directional light
    glm::vec3 lightP = glm::vec3(0.0f, 10.0f, 0.0f);
    glm::vec3 lightC = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 lightRot = glm::vec3(0.0f, 0.0f, 0.0f);
    for (const auto& obj : sceneObjects) {
        if (obj.type == ObjectType::LIGHT) {
            lightP = obj.position;
            lightC = glm::vec3(obj.color);
            lightRot = obj.rotation;
            break;
        }
    }

    // Calculate light forward and up vectors based on rotation
    glm::mat4 rotM = glm::mat4(1.0f);
    rotM = glm::rotate(rotM, glm::radians(lightRot.x), glm::vec3(1, 0, 0));
    rotM = glm::rotate(rotM, glm::radians(lightRot.y), glm::vec3(0, 1, 0));
    rotM = glm::rotate(rotM, glm::radians(lightRot.z), glm::vec3(0, 0, 1));
    
    // Default light direction is down (0, -1, 0)
    glm::vec3 lightDir = glm::normalize(glm::vec3(rotM * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
    // Default up vector for the light camera is forward (0, 0, -1) to prevent singularity
    glm::vec3 lightUp = glm::normalize(glm::vec3(rotM * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

    // Update Light Space Matrix
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, -20.0f, 50.0f);
    lightProjection[1][1] *= -1;
    glm::mat4 lightView = glm::lookAt(lightP, lightP + lightDir, lightUp);
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    ubo.lightPos = lightP;
    ubo.lightColor = lightC;
    ubo.lightSpaceMatrix = lightSpaceMatrix;
    ubo.lightDir = lightDir;
    ubo.enableShadows = enableShadowMapping ? 1.0f : 0.0f;
    ubo.viewPos = camPos;
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));

    // Game (Main) Camera UBO
    UniformBufferObject gameUbo{};
    gameUbo.lightPos = lightP;
    gameUbo.lightColor = lightC;
    gameUbo.lightSpaceMatrix = lightSpaceMatrix;
    gameUbo.lightDir = lightDir;
    gameUbo.enableShadows = enableShadowMapping ? 1.0f : 0.0f;
    gameUbo.viewPos = mainCameraPos;
    gameUbo.view = glm::lookAt(mainCameraPos, mainCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    gameUbo.proj = glm::perspective(glm::radians(mainCameraFov),
        (float)WIDTH / (float)HEIGHT, mainCameraNear, mainCameraFar);
    gameUbo.proj[1][1] *= -1;
    memcpy(gameUniformBuffersMapped[currentImage], &gameUbo, sizeof(gameUbo));
}

void VulkanApp::drawFrame()
{
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapChain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    // Update physics in Play mode
    if (mode == AppMode::PLAY)
    {
        updatePhysics(ImGui::GetIO().DeltaTime);
    }

    updateUniformBuffer(currentFrame);

    updateProfilerMetrics(ImGui::GetIO().DeltaTime);
    renderImGuiUI();

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // SHADOW PASS
    VkRenderPassBeginInfo shadowPassInfo{};
    shadowPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    shadowPassInfo.renderPass = shadowRenderPass;
    shadowPassInfo.framebuffer = shadowFramebuffer;
    shadowPassInfo.renderArea.offset = {0, 0};
    shadowPassInfo.renderArea.extent = {2048, 2048};

    VkClearValue shadowClearValues{};
    shadowClearValues.depthStencil = {1.0f, 0};
    shadowPassInfo.clearValueCount = 1;
    shadowPassInfo.pClearValues = &shadowClearValues;

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &shadowPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

    VkViewport shadowViewport{};
    shadowViewport.x = 0.0f;
    shadowViewport.y = 0.0f;
    shadowViewport.width = 2048.0f;
    shadowViewport.height = 2048.0f;
    shadowViewport.minDepth = 0.0f;
    shadowViewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &shadowViewport);

    VkRect2D shadowScissor{};
    shadowScissor.offset = {0, 0};
    shadowScissor.extent = {2048, 2048};
    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &shadowScissor);
    
    // Depth bias to prevent acne
    vkCmdSetDepthBias(commandBuffers[currentFrame], 1.25f, 0.0f, 1.75f);

    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    // Draw objects into shadow map
    if (enableShadowMapping) {
        for (const auto& obj : sceneObjects) {
        if (obj.meshId >= 0 && obj.meshId < meshes.size()) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, obj.scale);

            vkCmdPushConstants(commandBuffers[currentFrame], shadowPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);

            VkBuffer vertexBuffers[] = {meshes[obj.meshId].vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[currentFrame], meshes[obj.meshId].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffers[currentFrame], static_cast<uint32_t>(meshes[obj.meshId].indices.size()), 1, 0, 0, 0);
        }
        }
    }
    vkCmdEndRenderPass(commandBuffers[currentFrame]);

    // 1. Offscreen Render Pass (Draw 3D Scene)
    VkRenderPassBeginInfo offscreenInfo{};
    offscreenInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    offscreenInfo.renderPass = offscreenRenderPass;
    offscreenInfo.framebuffer = offscreenFramebuffer;
    offscreenInfo.renderArea.offset = {0, 0};
    offscreenInfo.renderArea.extent = {WIDTH, HEIGHT};

    std::array<VkClearValue, 2> offscreenClearValues{};
    offscreenClearValues[0].color = {{backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f}};
    offscreenClearValues[1].depthStencil = {1.0f, 0};
    offscreenInfo.clearValueCount = static_cast<uint32_t>(offscreenClearValues.size());
    offscreenInfo.pClearValues = offscreenClearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &offscreenInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)WIDTH;
    viewport.height = (float)HEIGHT;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {WIDTH, HEIGHT};
    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

    // Bind UBO Set (Set 0)
    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    for (size_t i = 0; i < sceneObjects.size(); ++i) {
        VkDescriptorSet texSet = defaultTexture.descriptorSet;
        if (sceneObjects[i].textureId >= 0 && sceneObjects[i].textureId < textures.size()) {
            texSet = textures[sceneObjects[i].textureId].descriptorSet;
        }
        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texSet, 0, nullptr);

        if (sceneObjects[i].meshId >= 0 && sceneObjects[i].meshId < meshes.size()) {
            PushConstants push{};
            push.model = getWorldMatrix(sceneObjects, i);
            vkCmdPushConstants(commandBuffers[currentFrame], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);

            VkBuffer vertexBuffers[] = { meshes[sceneObjects[i].meshId].vertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffers[currentFrame], meshes[sceneObjects[i].meshId].indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(commandBuffers[currentFrame], meshes[sceneObjects[i].meshId].indexCount, 1, 0, 0, 0);
        }
    }
    vkCmdEndRenderPass(commandBuffers[currentFrame]);

    // 2. Game View Render Pass (Main Camera)
    VkRenderPassBeginInfo gamePassInfo{};
    gamePassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    gamePassInfo.renderPass = offscreenRenderPass;
    gamePassInfo.framebuffer = gameFramebuffer;
    gamePassInfo.renderArea.offset = {0, 0};
    gamePassInfo.renderArea.extent = {WIDTH, HEIGHT};

    std::array<VkClearValue, 2> gameClearValues{};
    gameClearValues[0].color = {{backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f}};
    gameClearValues[1].depthStencil = {1.0f, 0};
    gamePassInfo.clearValueCount = static_cast<uint32_t>(gameClearValues.size());
    gamePassInfo.pClearValues = gameClearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &gamePassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

    // Bind GAME camera UBO (Set 0)
    vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &gameDescriptorSets[currentFrame], 0, nullptr);

    for (size_t i = 0; i < sceneObjects.size(); ++i) {
        VkDescriptorSet texSet = defaultTexture.descriptorSet;
        if (sceneObjects[i].textureId >= 0 && sceneObjects[i].textureId < textures.size()) {
            texSet = textures[sceneObjects[i].textureId].descriptorSet;
        }
        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &texSet, 0, nullptr);

        if (sceneObjects[i].meshId >= 0 && sceneObjects[i].meshId < meshes.size()) {
            PushConstants push{};
            push.model = getWorldMatrix(sceneObjects, i);
            vkCmdPushConstants(commandBuffers[currentFrame], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);
            VkBuffer vbs[] = { meshes[sceneObjects[i].meshId].vertexBuffer };
            VkDeviceSize offs[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vbs, offs);
            vkCmdBindIndexBuffer(commandBuffers[currentFrame], meshes[sceneObjects[i].meshId].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffers[currentFrame], meshes[sceneObjects[i].meshId].indexCount, 1, 0, 0, 0);
        }
    }
    vkCmdEndRenderPass(commandBuffers[currentFrame]);

    // 3. Main Render Pass (Draw ImGui only)
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;
    
    // Clear with a dark background for the borders outside ImGui panels
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);
    vkCmdEndRenderPass(commandBuffers[currentFrame]);

    if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanApp::cleanupSwapChain()
{
    if (depthImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, depthImageView, nullptr);
    }
    if (depthImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, depthImage, nullptr);
    }
    if (depthImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, depthImageMemory, nullptr);
    }

    for (auto framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    if (swapChain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }
}

void VulkanApp::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

void VulkanApp::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void VulkanApp::cleanupImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (imguiDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
    }
}

void VulkanApp::cleanup()
{
    cleanupImGui();
    cleanupSwapChain();

    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
    if (graphicsPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
    }
    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        }
        if (uniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }

    if (descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    if (uboSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, uboSetLayout, nullptr);
    }
    if (textureSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, textureSetLayout, nullptr);
    }

    if (indexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, indexBuffer, nullptr);
    }
    if (indexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, indexBufferMemory, nullptr);
    }

    if (vertexBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, vertexBuffer, nullptr);
    }
    if (vertexBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }

    // Cleanup procedural meshes
    for (auto& mesh : meshes)
    {
        if (mesh.vertexBuffer != VK_NULL_HANDLE)
            vkDestroyBuffer(device, mesh.vertexBuffer, nullptr);
        if (mesh.vertexBufferMemory != VK_NULL_HANDLE)
            vkFreeMemory(device, mesh.vertexBufferMemory, nullptr);
        if (mesh.indexBuffer != VK_NULL_HANDLE)
            vkDestroyBuffer(device, mesh.indexBuffer, nullptr);
        if (mesh.indexBufferMemory != VK_NULL_HANDLE)
            vkFreeMemory(device, mesh.indexBufferMemory, nullptr);
    }
    meshes.clear();

    // Cleanup game view resources
    if (gameFramebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(device, gameFramebuffer, nullptr);
    if (gameColorImageView != VK_NULL_HANDLE) vkDestroyImageView(device, gameColorImageView, nullptr);
    if (gameColorImage != VK_NULL_HANDLE) vkDestroyImage(device, gameColorImage, nullptr);
    if (gameColorImageMemory != VK_NULL_HANDLE) vkFreeMemory(device, gameColorImageMemory, nullptr);
    if (gameDepthImageView != VK_NULL_HANDLE) vkDestroyImageView(device, gameDepthImageView, nullptr);
    if (gameDepthImage != VK_NULL_HANDLE) vkDestroyImage(device, gameDepthImage, nullptr);
    if (gameDepthImageMemory != VK_NULL_HANDLE) vkFreeMemory(device, gameDepthImageMemory, nullptr);
    if (gameSampler != VK_NULL_HANDLE) vkDestroySampler(device, gameSampler, nullptr);
    for (size_t i = 0; i < gameUniformBuffers.size(); i++) {
        if (gameUniformBuffers[i] != VK_NULL_HANDLE) vkDestroyBuffer(device, gameUniformBuffers[i], nullptr);
        if (gameUniformBuffersMemory[i] != VK_NULL_HANDLE) vkFreeMemory(device, gameUniformBuffersMemory[i], nullptr);
    }


    // Cleanup textures
    if (defaultTexture.image != VK_NULL_HANDLE) {
        vkDestroyImageView(device, defaultTexture.view, nullptr);
        vkDestroyImage(device, defaultTexture.image, nullptr);
        vkFreeMemory(device, defaultTexture.memory, nullptr);
    }
    for (auto& tex : textures) {
        if (tex.image != VK_NULL_HANDLE) {
            vkDestroyImageView(device, tex.view, nullptr);
            vkDestroyImage(device, tex.image, nullptr);
            vkFreeMemory(device, tex.memory, nullptr);
        }
    }
    textures.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (imageAvailableSemaphores[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        }
        if (renderFinishedSemaphores[i] != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        }
        if (inFlightFences[i] != VK_NULL_HANDLE)
        {
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
    }

    if (commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    if (device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
    }

    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }

    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
    }

    if (window)
    {
        glfwDestroyWindow(window);
    }

    glfwTerminate();
}


void VulkanApp::createShadowRenderPass() {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 0;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &depthAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &shadowRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow render pass!");
    }
}

void VulkanApp::createShadowResources() {
    createImage(2048, 2048, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowImage, shadowImageMemory);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = shadowImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &shadowImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow image view!");
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &shadowSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow sampler!");
    }

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = shadowRenderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &shadowImageView;
    framebufferInfo.width = 2048;
    framebufferInfo.height = 2048;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &shadowFramebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow framebuffer!");
    }
}

void VulkanApp::createShadowPipeline() {
    auto vertShaderCode = readFile("shadow_vert.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 2048.0f;
    viewport.height = 2048.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { 2048, 2048 };

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; // Render both faces to avoid peter-panning
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;

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

    std::array<VkPushConstantRange, 1> pushConstantRange{};
    pushConstantRange[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange[0].offset = 0;
    pushConstantRange[0].size = sizeof(glm::mat4); // model

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &uboSetLayout; // Reuse main UBO layout
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRange.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &shadowPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow pipeline layout!");
    }
    
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 1;
    pipelineInfo.pStages = &vertShaderStageInfo;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = shadowPipelineLayout;
    pipelineInfo.renderPass = shadowRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.pDynamicState = &dynamicStateInfo;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shadowPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow graphics pipeline!");
    }

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}
