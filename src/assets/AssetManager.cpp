#include "assets/AssetManager.h"
#include "assets/PrimitiveGenerator.h"
#include "assets/ModelLoader.h"
#include "renderer/VulkanRenderer.h"
#include "vendor/stb_image.h"

#include <stdexcept>
#include <cstring>
#include <iostream>

AssetManager::AssetManager()
{
}

AssetManager::~AssetManager()
{
}

void AssetManager::init(VulkanRenderer* r)
{
    renderer = r;
    createTextureSampler();
    createDefaultTexture();

    primitiveCubeMeshId     = createCubeMesh();
    primitiveSphereMeshId   = createSphereMesh();
    primitivePlaneMeshId    = createPlaneMesh();
    primitiveCylinderMeshId = createCylinderMesh(0.5f, 0.5f, 1.0f, 16);
    primitiveConeMeshId     = createConeMesh(0.5f, 1.0f, 16);
}

void AssetManager::cleanup()
{
    if (!renderer) return;
    VkDevice device = renderer->getDevice();
    if (device == VK_NULL_HANDLE) return;

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

    if (textureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
    }

    if (defaultTexture.image != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, defaultTexture.view, nullptr);
        vkDestroyImage(device, defaultTexture.image, nullptr);
        vkFreeMemory(device, defaultTexture.memory, nullptr);
    }

    for (auto& tex : textures)
    {
        if (tex.image != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, tex.view, nullptr);
            vkDestroyImage(device, tex.image, nullptr);
            vkFreeMemory(device, tex.memory, nullptr);
        }
    }
    textures.clear();

    for (auto& [path, tex] : assetThumbnails)
    {
        if (tex.image != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, tex.view, nullptr);
            vkDestroyImage(device, tex.image, nullptr);
            vkFreeMemory(device, tex.memory, nullptr);
        }
    }
    assetThumbnails.clear();
}

void AssetManager::createMeshBuffers(Mesh& mesh)
{
    if (!renderer) return;
    VkDevice device = renderer->getDevice();

    VkDeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
    VkBuffer stagingVertexBuffer;
    VkDeviceMemory stagingVertexBufferMemory;
    renderer->createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           stagingVertexBuffer, stagingVertexBufferMemory);

    void* data;
    vkMapMemory(device, stagingVertexBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, mesh.vertices.data(), (size_t)vertexBufferSize);
    vkUnmapMemory(device, stagingVertexBufferMemory);

    renderer->createBuffer(vertexBufferSize,
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           mesh.vertexBuffer, mesh.vertexBufferMemory);

    renderer->copyBuffer(stagingVertexBuffer, mesh.vertexBuffer, vertexBufferSize);
    vkDestroyBuffer(device, stagingVertexBuffer, nullptr);
    vkFreeMemory(device, stagingVertexBufferMemory, nullptr);

    VkDeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    VkBuffer stagingIndexBuffer;
    VkDeviceMemory stagingIndexBufferMemory;
    renderer->createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           stagingIndexBuffer, stagingIndexBufferMemory);

    vkMapMemory(device, stagingIndexBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, mesh.indices.data(), (size_t)indexBufferSize);
    vkUnmapMemory(device, stagingIndexBufferMemory);

    renderer->createBuffer(indexBufferSize,
                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           mesh.indexBuffer, mesh.indexBufferMemory);

    renderer->copyBuffer(stagingIndexBuffer, mesh.indexBuffer, indexBufferSize);
    vkDestroyBuffer(device, stagingIndexBuffer, nullptr);
    vkFreeMemory(device, stagingIndexBufferMemory, nullptr);
}

void AssetManager::createTextureFromRawPixels(const unsigned char* pixels, int texWidth, int texHeight, Texture& texture)
{
    if (!renderer) return;
    VkDevice device = renderer->getDevice();
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    renderer->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                           stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    renderer->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          texture.image, texture.memory);

    renderer->transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    renderer->copyBufferToImage(stagingBuffer, texture.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    renderer->transitionImageLayout(texture.image, VK_FORMAT_R8G8B8A8_SRGB,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    texture.view = renderer->createImageView(texture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    texture.descriptorSet = renderer->createTextureDescriptorSet(texture.view, textureSampler);
}

void AssetManager::loadTexture(const std::string& path, Texture& texture)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error("Failed to load texture image: " + path);
    }
    createTextureFromRawPixels(pixels, texWidth, texHeight, texture);
    stbi_image_free(pixels);
}

void AssetManager::createTextureSampler()
{
    if (!renderer) return;
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(renderer->getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

void AssetManager::createDefaultTexture()
{
    const unsigned char whitePixel[4] = { 255, 255, 255, 255 };
    createTextureFromRawPixels(whitePixel, 1, 1, defaultTexture);
}

int AssetManager::createCubeMesh()
{
    Mesh mesh = PrimitiveGenerator::generateCube();
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int AssetManager::createSphereMesh(int stacks, int slices)
{
    Mesh mesh = PrimitiveGenerator::generateSphere(stacks, slices);
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int AssetManager::createPlaneMesh()
{
    Mesh mesh = PrimitiveGenerator::generatePlane();
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int AssetManager::createCylinderMesh(float radiusTop, float radiusBottom, float height, int slices)
{
    Mesh mesh = PrimitiveGenerator::generateCylinder(radiusTop, radiusBottom, height, slices);
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int AssetManager::createConeMesh(float radius, float height, int slices)
{
    Mesh mesh = PrimitiveGenerator::generateCone(radius, height, slices);
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int AssetManager::createTerrainMesh(int gridW, int gridD, float cellSize, float heightScale, int seed, int biomeType)
{
    Mesh mesh = PrimitiveGenerator::generateTerrain(gridW, gridD, cellSize, heightScale, seed, biomeType);
    createMeshBuffers(mesh);
    meshes.push_back(std::move(mesh));
    return static_cast<int>(meshes.size()) - 1;
}

int AssetManager::getOrLoadModelAsset(const std::string& pathStr)
{
    if (cachedModelMeshIds.find(pathStr) != cachedModelMeshIds.end())
    {
        return cachedModelMeshIds[pathStr];
    }
    try {
        Mesh m;
        ModelLoader::loadModel(pathStr, m, this);
        createMeshBuffers(m);
        meshes.push_back(std::move(m));
        int id = static_cast<int>(meshes.size()) - 1;
        cachedModelMeshIds[pathStr] = id;
        return id;
    } catch (const std::exception& e) {
        printf("[SceneGen] Failed to load model %s: %s\n", pathStr.c_str(), e.what());
        return -1;
    }
}

int AssetManager::getOrLoadTextureAsset(const std::string& pathStr)
{
    if (cachedTextureIds.find(pathStr) != cachedTextureIds.end())
    {
        return cachedTextureIds[pathStr];
    }
    try {
        Texture tex;
        loadTexture(pathStr, tex);
        textures.push_back(tex);
        int id = static_cast<int>(textures.size()) - 1;
        cachedTextureIds[pathStr] = id;
        return id;
    } catch (const std::exception& e) {
        printf("[SceneGen] Failed to load texture %s: %s\n", pathStr.c_str(), e.what());
        return -1;
    }
}

int AssetManager::load3DModelAsset(const std::string& filePath)
{
    try {
        Mesh newMesh;
        ModelLoader::loadModel(filePath, newMesh, this);
        createMeshBuffers(newMesh);
        meshes.push_back(newMesh);
        return static_cast<int>(meshes.size()) - 1;
    }
    catch (const std::exception& e) {
        printf("Error loading 3D Model Asset (%s): %s\n", filePath.c_str(), e.what());
        return -1;
    }
}

int AssetManager::loadTextureAsset(const std::string& filePath)
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

Mesh* AssetManager::getMesh(int id)
{
    if (id >= 0 && id < static_cast<int>(meshes.size()))
        return &meshes[id];
    return nullptr;
}

const Mesh* AssetManager::getMesh(int id) const
{
    if (id >= 0 && id < static_cast<int>(meshes.size()))
        return &meshes[id];
    return nullptr;
}

Texture* AssetManager::getTexture(int id)
{
    if (id >= 0 && id < static_cast<int>(textures.size()))
        return &textures[id];
    return nullptr;
}

const Texture* AssetManager::getTexture(int id) const
{
    if (id >= 0 && id < static_cast<int>(textures.size()))
        return &textures[id];
    return nullptr;
}
