#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <memory>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::ivec4 boneIDs = glm::ivec4(-1, -1, -1, -1);
    glm::vec4 weights = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions{};

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
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SINT;
        attributeDescriptions[4].offset = offsetof(Vertex, boneIDs);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vertex, weights);

        return attributeDescriptions;
    }
};

struct SubMesh
{
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    int textureId = -1;
    float roughness = 0.5f;
    float metallic = 0.0f;
    bool isFoliage = false;
    bool twoSided = false;
};

class Skeleton;
class AnimationClip;

struct Mesh
{
    std::string sourcePath = "";
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    uint32_t indexCount = 0;
    int defaultTextureId = -1;
    float defaultRoughness = 0.5f;
    float defaultMetallic = 0.0f;
    bool isFoliage = false;
    std::vector<SubMesh> submeshes;
    glm::vec3 pivotOffset = glm::vec3(0.0f);
    float targetScale = 1.0f;
    glm::vec3 originalMinB = glm::vec3(0.0f);
    glm::vec3 originalMaxB = glm::vec3(0.0f);

    bool isSkinned = false;
    std::shared_ptr<Skeleton> skeleton = nullptr;
    std::vector<std::shared_ptr<AnimationClip>> defaultClips;
};

struct Texture
{
    std::string sourcePath = "";
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};
