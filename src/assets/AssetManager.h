#pragma once

#include "core/Vertex.h"
#include <vector>
#include <string>
#include <unordered_map>

class VulkanRenderer;

class AssetManager
{
public:
    AssetManager();
    ~AssetManager();

    void init(VulkanRenderer* renderer);
    void cleanup();

    void createMeshBuffers(Mesh& mesh);
    void createTextureFromRawPixels(const unsigned char* pixels, int texWidth, int texHeight, Texture& texture);
    void loadTexture(const std::string& path, Texture& texture);
    void createTextureSampler();
    void createDefaultTexture();

    // Primitive meshes
    int createCubeMesh();
    int createSphereMesh(int stacks = 16, int slices = 32);
    int createPlaneMesh();
    int createCylinderMesh(float radiusTop = 0.5f, float radiusBottom = 0.5f, float height = 1.0f, int slices = 16);
    int createConeMesh(float radius = 0.5f, float height = 1.0f, int slices = 16);
    int createTerrainMesh(int gridW, int gridD, float cellSize, float heightScale, int seed, int biomeType);

    // Asset caching & loading
    int getOrLoadModelAsset(const std::string& pathStr);
    int getOrLoadTextureAsset(const std::string& pathStr);
    int load3DModelAsset(const std::string& filePath);
    int loadTextureAsset(const std::string& filePath);

    // Accessors
    std::vector<Mesh>& getMeshes() { return meshes; }
    const std::vector<Mesh>& getMeshes() const { return meshes; }
    Mesh* getMesh(int id);
    const Mesh* getMesh(int id) const;

    std::vector<Texture>& getTextures() { return textures; }
    const std::vector<Texture>& getTextures() const { return textures; }
    Texture* getTexture(int id);
    const Texture* getTexture(int id) const;

    const Texture& getDefaultTexture() const { return defaultTexture; }
    VkSampler getTextureSampler() const { return textureSampler; }

    int getCubeMeshId() const { return primitiveCubeMeshId; }
    int getSphereMeshId() const { return primitiveSphereMeshId; }
    int getPlaneMeshId() const { return primitivePlaneMeshId; }
    int getCylinderMeshId() const { return primitiveCylinderMeshId; }
    int getConeMeshId() const { return primitiveConeMeshId; }

    std::unordered_map<std::string, Texture>& getThumbnails() { return assetThumbnails; }
    std::unordered_map<std::string, int>& getCachedTextureIds() { return cachedTextureIds; }

private:
    VulkanRenderer* renderer = nullptr;

    std::vector<Mesh> meshes;
    std::vector<Texture> textures;
    Texture defaultTexture;
    VkSampler textureSampler = VK_NULL_HANDLE;

    int primitiveCubeMeshId = -1;
    int primitiveSphereMeshId = -1;
    int primitivePlaneMeshId = -1;
    int primitiveCylinderMeshId = -1;
    int primitiveConeMeshId = -1;

    std::unordered_map<std::string, int> cachedModelMeshIds;
    std::unordered_map<std::string, int> cachedTextureIds;
    std::unordered_map<std::string, Texture> assetThumbnails;
};
