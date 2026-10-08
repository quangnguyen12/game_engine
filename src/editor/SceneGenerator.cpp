#include "editor/SceneGenerator.h"
#include "editor/GizmoSystem.h"
#include "scene/Scene.h"
#include "assets/AssetManager.h"
#include "imgui.h"

#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <functional>
#include <cmath>
#include <algorithm>

static std::vector<std::string> scanAvailable3DModels()
{
    std::vector<std::string> modelFiles;
    std::vector<std::string> searchDirs = { "assets", "assets/models" };
    for (const auto& dir : searchDirs)
    {
        if (std::filesystem::exists(dir))
        {
            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (entry.is_regular_file())
                {
                    std::string stem = entry.path().stem().string();
                    std::string ext = entry.path().extension().string();
                    for (auto& c : ext) c = static_cast<char>(tolower(c));
                    if (stem == "trees9") continue;
                    if (ext == ".glb" || ext == ".gltf" || ext == ".obj")
                    {
                        modelFiles.push_back(entry.path().string());
                    }
                }
            }
        }
    }
    return modelFiles;
}

SceneGenerator::SceneGenerator()
    : showSceneGeneratorPanel(true),
      genUseReal3DModels(true),
      genPreset(0),
      genSeed(42),
      genGridSize(35),
      genHeightScale(5.0f),
      genDensity(1.0f),
      genIncludeTrees(true),
      genIncludeRocks(true),
      genIncludeBuildings(true),
      genIncludeLights(true),
      genClearExisting(true),
      genSunColor(1.0f, 0.92f, 0.75f),
      genSunIntensity(1.5f)
{
}

void SceneGenerator::generate3DScene(Scene* scene, AssetManager* assetManager)
{
    if (!scene || !assetManager) return;

    scene->saveHistory();
    auto& sceneObjects = scene->getObjects();
    if (genClearExisting)
    {
        sceneObjects.clear();
        scene->setSelectedObjectIndex(-1);
    }

    int primitiveCubeMeshId = assetManager->getCubeMeshId();
    int primitiveSphereMeshId = assetManager->getSphereMeshId();
    int primitivePlaneMeshId = assetManager->getPlaneMeshId();
    int primitiveCylinderMeshId = assetManager->getCylinderMeshId();
    int primitiveConeMeshId = assetManager->getConeMeshId();

    bool sunExists = false;
    for (const auto& obj : sceneObjects)
    {
        if (obj.type == ObjectType::LIGHT) { sunExists = true; break; }
    }

    if (!sunExists)
    {
        SceneObject sun;
        sun.id = static_cast<int>(sceneObjects.size());
        sun.name = "☀️ Sun Light (Main)";
        sun.type = ObjectType::LIGHT;
        sun.position = glm::vec3(0.0f, 15.0f, 10.0f);
        sun.rotation = glm::vec3(45.0f, 30.0f, 0.0f);
        sun.scale = glm::vec3(0.5f, 0.5f, 0.5f);
        sun.color = glm::vec4(genSunColor, 1.0f);
        sun.meshId = primitiveCubeMeshId;
        auto lightComp = std::make_shared<LightComponent>();
        lightComp->color = sun.color;
        lightComp->intensity = genSunIntensity;
        sun.components.push_back(lightComp);
        sceneObjects.push_back(sun);
    }

    std::mt19937 rng(genSeed);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
    std::uniform_real_distribution<float> distRot(0.0f, 360.0f);

    float cellSize = 1.0f;
    float halfW = (genGridSize - 1) * cellSize * 0.5f;

    std::vector<std::string> realModels = scanAvailable3DModels();

    auto sampleHeight = [&](float x, float z) -> float {
        float sx = x * 0.1f + genSeed * 0.13f;
        float sz = z * 0.1f + genSeed * 0.17f;
        float h = 0.0f;
        if (genPreset == 0) { // Forest
            h += std::sin(sx) * std::cos(sz) * 0.6f;
            h += std::sin(sx * 2.3f + 1.2f) * std::cos(sz * 2.1f + 0.5f) * 0.3f;
            h += std::sin(sx * 4.7f) * std::cos(sz * 4.9f) * 0.1f;
        } else if (genPreset == 1) { // Desert
            h += std::sin(sx * 0.7f + sz * 0.5f) * 0.8f;
            h += std::cos(sx * 1.8f - sz * 1.2f) * 0.2f;
        } else if (genPreset == 3) { // Medieval Valley
            float distFromCenter = std::sqrt(x * x + z * z);
            h = (1.0f - std::exp(-distFromCenter * 0.05f)) * 0.8f;
            h += std::sin(sx * 1.5f) * std::cos(sz * 1.5f) * 0.2f;
        } else if (genPreset == 4) { // Floating Islands
            h = std::sin(sx * 1.2f) * std::cos(sz * 1.2f) * 1.2f;
        }
        return h * genHeightScale;
    };

    auto sampleNormal = [&](float x, float z) -> glm::vec3 {
        float eps = 0.25f;
        float hL = sampleHeight(x - eps, z);
        float hR = sampleHeight(x + eps, z);
        float hD = sampleHeight(x, z - eps);
        float hU = sampleHeight(x, z + eps);
        glm::vec3 n(-(hR - hL) / (2.0f * eps), 1.0f, -(hU - hD) / (2.0f * eps));
        return glm::normalize(n);
    };

    auto setupRockTransform = [&](SceneObject& rockObj, float rx, float ry, float rz, float baseScale) {
        float yaw = distRot(rng);
        rockObj.rotation = glm::vec3(0.0f, yaw, 0.0f);
        float flatFactor = 0.22f + dist01(rng) * 0.08f;
        float spreadX = baseScale * (1.10f + dist01(rng) * 0.20f);
        float spreadZ = baseScale * (1.10f + dist01(rng) * 0.20f);
        float heightY = baseScale * flatFactor;
        float embed = heightY * 0.30f;
        rockObj.scale = glm::vec3(spreadX, heightY, spreadZ);
        rockObj.position = glm::vec3(rx, ry - embed, rz);
    };

    const auto& meshes = assetManager->getMeshes();

    if (genPreset == 0) // Forest
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale, genSeed, 0);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🏔️ Terrain_Landscape";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        terrain.color = glm::vec4(1.0f);

        if (std::filesystem::exists("assets/grass.jpg"))
        {
            terrain.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
        }
        else if (std::filesystem::exists("assets/textures/grass.jpg"))
        {
            terrain.textureId = assetManager->getOrLoadTextureAsset("assets/textures/grass.jpg");
        }
        else if (std::filesystem::exists("assets/Texture/grass.jpg"))
        {
            terrain.textureId = assetManager->getOrLoadTextureAsset("assets/Texture/grass.jpg");
        }
        sceneObjects.push_back(terrain);

        int count = static_cast<int>(genGridSize * genGridSize * 0.06f * genDensity);
        for (int i = 0; i < count; ++i)
        {
            float rx = (dist01(rng) * 2.0f - 1.0f) * (halfW - 2.0f);
            float rz = (dist01(rng) * 2.0f - 1.0f) * (halfW - 2.0f);
            float ry = sampleHeight(rx, rz);

            if (genUseReal3DModels && !realModels.empty() && dist01(rng) < 0.4f)
            {
                std::string chosenPath = realModels[rng() % realModels.size()];
                int mId = assetManager->getOrLoadModelAsset(chosenPath);
                if (mId >= 0)
                {
                    std::filesystem::path p(chosenPath);
                    std::string modelName = p.stem().string();

                    SceneObject realObj;
                    realObj.id = static_cast<int>(sceneObjects.size());
                    realObj.name = "🌐 " + modelName + "_" + std::to_string(i);
                    realObj.type = ObjectType::CUBE;
                    if (modelName.find("rock") != std::string::npos || modelName.find("Rock") != std::string::npos)
                    {
                        glm::vec3 n = sampleNormal(rx, rz);
                        if (n.y < 0.88f) continue;
                        float rockScale = 0.22f + dist01(rng) * 0.22f;
                        setupRockTransform(realObj, rx, ry, rz, rockScale);
                    }
                    else
                    {
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        float sc = 1.0f;
                        if (modelName == "Fox") sc = 0.02f;
                        else if (modelName == "Duck") sc = 0.5f;
                        else if (modelName == "Lantern") sc = 0.2f;
                        else if (modelName == "DamagedHelmet") sc = 0.4f;
                        else if (modelName == "CesiumMan") sc = 1.0f;
                        else if (modelName == "Avocado") sc = 4.0f;
                        else if (modelName == "PineTree" || modelName == "Tree" || modelName.find("tree") != std::string::npos || modelName.find("Tree") != std::string::npos) sc = 1.0f;

                        bool snapGround = (modelName.find("tree") != std::string::npos || modelName.find("Tree") != std::string::npos);
                        realObj.position = glm::vec3(rx, snapGround ? ry : (ry + 0.5f), rz);
                        realObj.scale = glm::vec3(sc);
                    }
                    realObj.color = glm::vec4(1.0f);
                    realObj.meshId = mId;
                    if (mId >= 0 && mId < static_cast<int>(meshes.size())) {
                        if (meshes[mId].submeshes.empty()) {
                            realObj.textureId = meshes[mId].defaultTextureId;
                        } else {
                            realObj.textureId = -1;
                        }
                        realObj.roughness = meshes[mId].defaultRoughness;
                        realObj.metallic = meshes[mId].defaultMetallic;
                    }
                    sceneObjects.push_back(realObj);
                    continue;
                }
            }

            if (genIncludeRocks && dist01(rng) < 0.20f)
            {
                glm::vec3 n = sampleNormal(rx, rz);
                if (n.y >= 0.88f && ry <= genHeightScale * 0.45f)
                {
                    int rockMeshId = -1;
                    if (std::filesystem::exists("assets/models/rock.glb")) rockMeshId = assetManager->getOrLoadModelAsset("assets/models/rock.glb");
                    else if (std::filesystem::exists("assets/rock.glb")) rockMeshId = assetManager->getOrLoadModelAsset("assets/rock.glb");

                    if (rockMeshId >= 0)
                    {
                        float rockScale = 0.18f + dist01(rng) * 0.22f;
                        SceneObject rock;
                        rock.id = static_cast<int>(sceneObjects.size());
                        rock.name = "🪨 Valley_FlatRock_" + std::to_string(i);
                        rock.type = ObjectType::CUBE;
                        setupRockTransform(rock, rx, ry, rz, rockScale);
                        rock.color = glm::vec4(1.0f);
                        rock.meshId = rockMeshId;
                        if (rockMeshId < static_cast<int>(meshes.size())) {
                            rock.textureId = meshes[rockMeshId].defaultTextureId;
                            rock.roughness = meshes[rockMeshId].defaultRoughness;
                            rock.metallic = meshes[rockMeshId].defaultMetallic;
                        }
                        sceneObjects.push_back(rock);
                        continue;
                    }
                }
            }

            if (genIncludeTrees && ry < genHeightScale * 0.55f)
            {
                float treeScale = 0.85f + dist01(rng) * 0.35f;

                static const std::vector<std::string> treeVariantPool = {
                    "assets/models/tree9.obj",
                    "assets/models/tree_mossy.obj",
                    "assets/models/tree_cedar.obj",
                    "assets/models/tree_pine.obj",
                    "assets/models/tree_savanna.obj",
                    "assets/models/tree_redwood.obj",
                    "assets/models/tree_oak.obj"
                };

                std::vector<std::string> availableTrees;
                for (const auto& tp : treeVariantPool) {
                    if (std::filesystem::exists(tp)) availableTrees.push_back(tp);
                }
                if (availableTrees.empty()) {
                    if (std::filesystem::exists("assets/models/PineTree.glb")) availableTrees.push_back("assets/models/PineTree.glb");
                }

                std::string treeModelPath = "";
                if (!availableTrees.empty()) {
                    treeModelPath = availableTrees[rng() % availableTrees.size()];
                }

                if (!treeModelPath.empty())
                {
                    int treeMeshId = assetManager->getOrLoadModelAsset(treeModelPath);
                    if (treeMeshId >= 0)
                    {
                        std::filesystem::path tp(treeModelPath);
                        std::string tStem = tp.stem().string();

                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌲 " + tStem + "_" + std::to_string(i);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(rx, ry, rz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(treeScale);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = treeMeshId;
                        if (treeMeshId < static_cast<int>(meshes.size())) {
                            if (meshes[treeMeshId].submeshes.empty()) {
                                if (meshes[treeMeshId].defaultTextureId >= 0) {
                                    treeObj.textureId = meshes[treeMeshId].defaultTextureId;
                                } else {
                                    int defTex = assetManager->getOrLoadTextureAsset("assets/textures/Tree_Leaves_Green.png");
                                    if (defTex < 0) defTex = assetManager->getOrLoadTextureAsset("assets/Texture/Walnut_L.png");
                                    if (defTex >= 0) treeObj.textureId = defTex;
                                }
                            } else {
                                treeObj.textureId = -1;
                            }
                        }
                        sceneObjects.push_back(treeObj);
                        continue;
                    }
                }

                SceneObject trunk;
                trunk.id = static_cast<int>(sceneObjects.size());
                trunk.name = "🌲 PineTree_Trunk_" + std::to_string(i);
                trunk.type = ObjectType::CUBE;
                trunk.position = glm::vec3(rx, ry + 0.6f * treeScale, rz);
                trunk.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                trunk.scale = glm::vec3(0.25f * treeScale, 1.2f * treeScale, 0.25f * treeScale);
                trunk.color = glm::vec4(1.0f);
                trunk.meshId = (primitiveCylinderMeshId >= 0) ? primitiveCylinderMeshId : primitiveCubeMeshId;
                trunk.textureId = assetManager->getOrLoadTextureAsset("assets/Texture/Bark___0.jpg");
                sceneObjects.push_back(trunk);

                SceneObject foliage1;
                foliage1.id = static_cast<int>(sceneObjects.size());
                foliage1.name = "🌲 PineTree_Foliage1_" + std::to_string(i);
                foliage1.type = ObjectType::CUBE;
                foliage1.position = glm::vec3(rx, ry + 1.6f * treeScale, rz);
                foliage1.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                foliage1.scale = glm::vec3(1.2f * treeScale, 1.0f * treeScale, 1.2f * treeScale);
                foliage1.color = glm::vec4(1.0f);
                foliage1.meshId = (primitiveConeMeshId >= 0) ? primitiveConeMeshId : primitiveSphereMeshId;
                foliage1.textureId = assetManager->getOrLoadTextureAsset("assets/Texture/Oak_Leav.jpg");
                sceneObjects.push_back(foliage1);

                SceneObject foliage2;
                foliage2.id = static_cast<int>(sceneObjects.size());
                foliage2.name = "🌲 PineTree_Foliage2_" + std::to_string(i);
                foliage2.type = ObjectType::CUBE;
                foliage2.position = glm::vec3(rx, ry + 2.3f * treeScale, rz);
                foliage2.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                foliage2.scale = glm::vec3(0.8f * treeScale, 0.8f * treeScale, 0.8f * treeScale);
                foliage2.color = glm::vec4(1.0f);
                foliage2.meshId = (primitiveConeMeshId >= 0) ? primitiveConeMeshId : primitiveSphereMeshId;
                foliage2.textureId = assetManager->getOrLoadTextureAsset("assets/Texture/Oak_Leav.jpg");
                sceneObjects.push_back(foliage2);
            }
            else if (genIncludeRocks)
            {
                glm::vec3 n = sampleNormal(rx, rz);
                if (n.y >= 0.88f && ry <= genHeightScale * 0.45f)
                {
                    int rockMeshId = -1;
                    if (std::filesystem::exists("assets/models/rock.glb")) {
                        rockMeshId = assetManager->getOrLoadModelAsset("assets/models/rock.glb");
                    } else if (std::filesystem::exists("assets/rock.glb")) {
                        rockMeshId = assetManager->getOrLoadModelAsset("assets/rock.glb");
                    }

                    if (rockMeshId >= 0)
                    {
                        float rockScale = 0.20f + dist01(rng) * 0.25f;
                        SceneObject rock;
                        rock.id = static_cast<int>(sceneObjects.size());
                        rock.name = "🪨 Meadow_FlatRock_" + std::to_string(i);
                        rock.type = ObjectType::CUBE;
                        setupRockTransform(rock, rx, ry, rz, rockScale);
                        rock.color = glm::vec4(1.0f);
                        rock.meshId = rockMeshId;
                        if (rockMeshId < static_cast<int>(meshes.size())) {
                            rock.textureId = meshes[rockMeshId].defaultTextureId;
                            rock.roughness = meshes[rockMeshId].defaultRoughness;
                            rock.metallic = meshes[rockMeshId].defaultMetallic;
                        }
                        sceneObjects.push_back(rock);
                    }
                    else
                    {
                        float rockScale = 0.35f + dist01(rng) * 0.4f;
                        SceneObject rock;
                        rock.id = static_cast<int>(sceneObjects.size());
                        rock.name = "🪨 Meadow_Boulder_" + std::to_string(i);
                        rock.type = ObjectType::SPHERE;
                        setupRockTransform(rock, rx, ry, rz, rockScale);
                        rock.color = glm::vec4(0.45f + dist01(rng) * 0.1f, 0.45f + dist01(rng) * 0.1f, 0.48f, 1.0f);
                        rock.meshId = primitiveSphereMeshId;
                        sceneObjects.push_back(rock);
                    }
                }
            }
        }
    }
    else if (genPreset == 1) // Desert
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale * 0.6f, genSeed, 1);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🏜️ Desert_Dunes";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        terrain.color = glm::vec4(0.9f, 0.75f, 0.4f, 1.0f);
        sceneObjects.push_back(terrain);

        if (genIncludeBuildings)
        {
            for (int step = 0; step < 6; ++step)
            {
                SceneObject pyramidLayer;
                pyramidLayer.id = static_cast<int>(sceneObjects.size());
                pyramidLayer.name = "🏛️ Ancient_Pyramid_Layer_" + std::to_string(step);
                pyramidLayer.type = ObjectType::CUBE;
                pyramidLayer.position = glm::vec3(0.0f, step * 0.8f + 0.4f, 0.0f);
                float width = (6 - step) * 1.6f;
                pyramidLayer.scale = glm::vec3(width, 0.8f, width);
                pyramidLayer.color = glm::vec4(0.85f - step * 0.03f, 0.68f - step * 0.03f, 0.38f, 1.0f);
                pyramidLayer.meshId = primitiveCubeMeshId;
                sceneObjects.push_back(pyramidLayer);
            }

            if (genUseReal3DModels && !realModels.empty())
            {
                int helmetMeshId = assetManager->getOrLoadModelAsset("assets/models/DamagedHelmet.glb");
                if (helmetMeshId >= 0)
                {
                    SceneObject relic;
                    relic.id = static_cast<int>(sceneObjects.size());
                    relic.name = "👑 Ancient_Relic_Helmet";
                    relic.type = ObjectType::CUBE;
                    relic.position = glm::vec3(0.0f, 5.2f, 0.0f);
                    relic.scale = glm::vec3(0.6f);
                    relic.color = glm::vec4(1.0f);
                    relic.meshId = helmetMeshId;
                    if (helmetMeshId < static_cast<int>(meshes.size())) {
                        relic.textureId = meshes[helmetMeshId].defaultTextureId;
                        relic.roughness = meshes[helmetMeshId].defaultRoughness;
                        relic.metallic = meshes[helmetMeshId].defaultMetallic;
                    }
                    sceneObjects.push_back(relic);
                }
            }
        }

        int count = static_cast<int>(20 * genDensity);
        for (int i = 0; i < count; ++i)
        {
            float rx = (dist01(rng) * 2.0f - 1.0f) * (halfW - 3.0f);
            float rz = (dist01(rng) * 2.0f - 1.0f) * (halfW - 3.0f);
            if (std::abs(rx) < 6.0f && std::abs(rz) < 6.0f) continue;
            float ry = sampleHeight(rx, rz);

            if (genUseReal3DModels && !realModels.empty() && dist01(rng) < 0.35f)
            {
                std::string chosenPath = realModels[rng() % realModels.size()];
                int mId = assetManager->getOrLoadModelAsset(chosenPath);
                if (mId >= 0)
                {
                    std::filesystem::path p(chosenPath);
                    std::string mName = p.stem().string();

                    SceneObject realObj;
                    realObj.id = static_cast<int>(sceneObjects.size());
                    realObj.name = "🌐 Desert_" + mName + "_" + std::to_string(i);
                    realObj.type = ObjectType::CUBE;
                    if (mName.find("rock") != std::string::npos || mName.find("Rock") != std::string::npos)
                    {
                        glm::vec3 n = sampleNormal(rx, rz);
                        if (n.y < 0.88f) continue;
                        float rockScale = 0.25f + dist01(rng) * 0.25f;
                        setupRockTransform(realObj, rx, ry, rz, rockScale);
                    }
                    else
                    {
                        float sc = 0.6f;
                        if (mName == "Fox") sc = 0.02f;
                        else if (mName == "Lantern") sc = 0.2f;

                        bool snapGround = (mName.find("tree") != std::string::npos || mName.find("Tree") != std::string::npos);
                        realObj.position = glm::vec3(rx, snapGround ? ry : (ry + 0.5f), rz);
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        realObj.scale = glm::vec3(sc);
                    }
                    realObj.color = glm::vec4(1.0f);
                    realObj.meshId = mId;
                    if (mId < static_cast<int>(meshes.size())) {
                        realObj.textureId = meshes[mId].defaultTextureId;
                        realObj.roughness = meshes[mId].defaultRoughness;
                        realObj.metallic = meshes[mId].defaultMetallic;
                    }
                    sceneObjects.push_back(realObj);
                    continue;
                }
            }

            SceneObject pillar;
            pillar.id = static_cast<int>(sceneObjects.size());
            pillar.name = "🏛️ Ruin_Pillar_" + std::to_string(i);
            pillar.type = ObjectType::CUBE;
            pillar.position = glm::vec3(rx, ry + 1.5f, rz);
            pillar.rotation = glm::vec3((dist01(rng) > 0.8f ? dist01(rng) * 25.0f : 0.0f), distRot(rng), 0.0f);
            pillar.scale = glm::vec3(0.6f, 3.0f, 0.6f);
            pillar.color = glm::vec4(0.78f, 0.65f, 0.45f, 1.0f);
            pillar.meshId = (primitiveCylinderMeshId >= 0) ? primitiveCylinderMeshId : primitiveCubeMeshId;
            sceneObjects.push_back(pillar);
        }
    }
    else if (genPreset == 2) // Cyberpunk City
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, 0.0f, genSeed, 2);
        SceneObject ground;
        ground.id = static_cast<int>(sceneObjects.size());
        ground.name = "🌃 City_Ground_Grid";
        ground.type = ObjectType::PLANE;
        ground.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ground.meshId = terrainMeshId;
        sceneObjects.push_back(ground);

        int cityBlocks = static_cast<int>(std::floor(genGridSize / 4));
        float blockStep = 3.5f;

        for (int bx = -cityBlocks / 2; bx <= cityBlocks / 2; ++bx)
        {
            for (int bz = -cityBlocks / 2; bz <= cityBlocks / 2; ++bz)
            {
                if (dist01(rng) < 0.2f) continue;

                float posX = bx * blockStep;
                float posZ = bz * blockStep;

                float bHeight = 3.0f + dist01(rng) * 18.0f * genHeightScale * 0.3f;
                float bWidth = 1.2f + dist01(rng) * 1.2f;

                SceneObject building;
                building.id = static_cast<int>(sceneObjects.size());
                building.name = "🏢 Skyscraper_" + std::to_string(bx) + "_" + std::to_string(bz);
                building.type = ObjectType::CUBE;
                building.position = glm::vec3(posX, bHeight * 0.5f, posZ);
                building.scale = glm::vec3(bWidth, bHeight, bWidth);

                float colR = 0.1f + dist01(rng) * 0.2f;
                float colG = 0.15f + dist01(rng) * 0.25f;
                float colB = 0.3f + dist01(rng) * 0.4f;
                building.color = glm::vec4(colR, colG, colB, 1.0f);
                building.meshId = primitiveCubeMeshId;
                sceneObjects.push_back(building);

                if (genIncludeLights && dist01(rng) > 0.5f)
                {
                    SceneObject neonLight;
                    neonLight.id = static_cast<int>(sceneObjects.size());
                    neonLight.name = "💡 Neon_Beacon_" + std::to_string(bx) + "_" + std::to_string(bz);
                    neonLight.type = ObjectType::LIGHT;
                    neonLight.position = glm::vec3(posX, bHeight + 0.8f, posZ);
                    neonLight.scale = glm::vec3(0.3f);

                    glm::vec3 neonColor = (dist01(rng) > 0.5f) ? glm::vec3(0.0f, 0.9f, 1.0f) : glm::vec3(1.0f, 0.1f, 0.8f);
                    neonLight.color = glm::vec4(neonColor, 1.0f);
                    neonLight.meshId = primitiveCubeMeshId;

                    auto lComp = std::make_shared<LightComponent>();
                    lComp->color = neonLight.color;
                    lComp->intensity = 2.5f;
                    neonLight.components.push_back(lComp);

                    sceneObjects.push_back(neonLight);
                }
            }
        }
    }
    else if (genPreset == 3) // Medieval Village
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale * 0.4f, genSeed, 3);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🏰 Citadel_Valley";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        sceneObjects.push_back(terrain);

        if (genIncludeBuildings)
        {
            SceneObject keep;
            keep.id = static_cast<int>(sceneObjects.size());
            keep.name = "🏰 Castle_Main_Keep";
            keep.type = ObjectType::CUBE;
            keep.position = glm::vec3(0.0f, 2.5f, 0.0f);
            keep.scale = glm::vec3(4.0f, 5.0f, 4.0f);
            keep.color = glm::vec4(0.55f, 0.55f, 0.58f, 1.0f);
            keep.meshId = primitiveCubeMeshId;
            sceneObjects.push_back(keep);

            glm::vec2 offsets[4] = { {-2.5f, -2.5f}, {2.5f, -2.5f}, {2.5f, 2.5f}, {-2.5f, 2.5f} };
            for (int t = 0; t < 4; ++t)
            {
                SceneObject tower;
                tower.id = static_cast<int>(sceneObjects.size());
                tower.name = "🏰 Watch_Tower_" + std::to_string(t);
                tower.type = ObjectType::CUBE;
                tower.position = glm::vec3(offsets[t].x, 3.2f, offsets[t].y);
                tower.scale = glm::vec3(1.2f, 6.4f, 1.2f);
                tower.color = glm::vec4(0.5f, 0.5f, 0.52f, 1.0f);
                tower.meshId = (primitiveCylinderMeshId >= 0) ? primitiveCylinderMeshId : primitiveCubeMeshId;
                sceneObjects.push_back(tower);

                SceneObject roof;
                roof.id = static_cast<int>(sceneObjects.size());
                roof.name = "🏰 Tower_Roof_" + std::to_string(t);
                roof.type = ObjectType::CUBE;
                roof.position = glm::vec3(offsets[t].x, 6.9f, offsets[t].y);
                roof.scale = glm::vec3(1.5f, 1.2f, 1.5f);
                roof.color = glm::vec4(0.7f, 0.15f, 0.12f, 1.0f);
                roof.meshId = (primitiveConeMeshId >= 0) ? primitiveConeMeshId : primitiveSphereMeshId;
                sceneObjects.push_back(roof);
            }

            if (genUseReal3DModels && !realModels.empty())
            {
                int charMeshId = assetManager->getOrLoadModelAsset("assets/models/CesiumMan.glb");
                if (charMeshId >= 0)
                {
                    SceneObject knight;
                    knight.id = static_cast<int>(sceneObjects.size());
                    knight.name = "🧍 Citadel_Guard_Knight";
                    knight.type = ObjectType::CUBE;
                    knight.position = glm::vec3(0.0f, 0.0f, 2.8f);
                    knight.rotation = glm::vec3(0.0f, 180.0f, 0.0f);
                    knight.scale = glm::vec3(1.0f);
                    knight.color = glm::vec4(1.0f);
                    knight.meshId = charMeshId;
                    if (charMeshId < static_cast<int>(meshes.size())) {
                        knight.textureId = meshes[charMeshId].defaultTextureId;
                        knight.roughness = meshes[charMeshId].defaultRoughness;
                        knight.metallic = meshes[charMeshId].defaultMetallic;
                    }
                    sceneObjects.push_back(knight);
                }
            }
        }

        int villageCount = static_cast<int>(15 * genDensity);
        for (int i = 0; i < villageCount; ++i)
        {
            float angle = (float(i) / float(villageCount)) * 6.28318f + dist01(rng) * 0.2f;
            float radius = 7.0f + dist01(rng) * 8.0f;
            float rx = std::cos(angle) * radius;
            float rz = std::sin(angle) * radius;
            float ry = sampleHeight(rx, rz);

            SceneObject house;
            house.id = static_cast<int>(sceneObjects.size());
            house.name = "🏡 Village_Cottage_" + std::to_string(i);
            house.type = ObjectType::CUBE;
            house.position = glm::vec3(rx, ry + 0.9f, rz);
            house.rotation = glm::vec3(0.0f, glm::degrees(angle) + 90.0f, 0.0f);
            house.scale = glm::vec3(1.6f, 1.8f, 2.2f);
            house.color = glm::vec4(0.75f, 0.65f, 0.52f, 1.0f);
            house.meshId = primitiveCubeMeshId;
            sceneObjects.push_back(house);

            SceneObject hRoof;
            hRoof.id = static_cast<int>(sceneObjects.size());
            hRoof.name = "🏡 Cottage_Roof_" + std::to_string(i);
            hRoof.type = ObjectType::CUBE;
            hRoof.position = glm::vec3(rx, ry + 2.3f, rz);
            hRoof.rotation = glm::vec3(0.0f, glm::degrees(angle) + 90.0f, 0.0f);
            hRoof.scale = glm::vec3(1.8f, 1.0f, 2.4f);
            hRoof.color = glm::vec4(0.55f, 0.25f, 0.15f, 1.0f);
            hRoof.meshId = (primitiveConeMeshId >= 0) ? primitiveConeMeshId : primitiveCubeMeshId;
            sceneObjects.push_back(hRoof);
        }
    }
    else if (genPreset == 4) // Floating Islands
    {
        int islandCount = static_cast<int>(7 * genDensity);
        for (int i = 0; i < islandCount; ++i)
        {
            float rx = (dist01(rng) * 2.0f - 1.0f) * (halfW * 0.8f);
            float rz = (dist01(rng) * 2.0f - 1.0f) * (halfW * 0.8f);
            float ry = (dist01(rng) * 2.0f - 1.0f) * 8.0f;
            float scaleXZ = 3.0f + dist01(rng) * 5.0f;

            SceneObject island;
            island.id = static_cast<int>(sceneObjects.size());
            island.name = "🏝️ Floating_Island_" + std::to_string(i);
            island.type = ObjectType::SPHERE;
            island.position = glm::vec3(rx, ry, rz);
            island.rotation = glm::vec3(dist01(rng) * 10.0f, distRot(rng), 0.0f);
            island.scale = glm::vec3(scaleXZ, 1.5f + dist01(rng) * 2.0f, scaleXZ);
            island.color = glm::vec4(0.35f, 0.48f, 0.3f, 1.0f);
            island.meshId = primitiveSphereMeshId;
            sceneObjects.push_back(island);

            if (genUseReal3DModels && !realModels.empty() && dist01(rng) < 0.5f)
            {
                std::string chosenPath = realModels[rng() % realModels.size()];
                int mId = assetManager->getOrLoadModelAsset(chosenPath);
                if (mId >= 0)
                {
                    std::filesystem::path p(chosenPath);
                    std::string mName = p.stem().string();

                    SceneObject realObj;
                    realObj.id = static_cast<int>(sceneObjects.size());
                    realObj.name = "🌐 Sky_" + mName + "_" + std::to_string(i);
                    realObj.type = ObjectType::CUBE;
                    if (mName.find("rock") != std::string::npos || mName.find("Rock") != std::string::npos) {
                        float rSc = 0.22f + dist01(rng) * 0.18f;
                        setupRockTransform(realObj, rx, ry + 0.75f, rz, rSc);
                    } else {
                        realObj.position = glm::vec3(rx, ry + 1.2f, rz);
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        float sc = (mName == "Fox") ? 0.02f : (mName == "Duck" ? 0.5f : 0.3f);
                        realObj.scale = glm::vec3(sc);
                    }
                    realObj.color = glm::vec4(1.0f);
                    realObj.meshId = mId;
                    if (mId < static_cast<int>(meshes.size())) {
                        realObj.textureId = meshes[mId].defaultTextureId;
                        realObj.roughness = meshes[mId].defaultRoughness;
                        realObj.metallic = meshes[mId].defaultMetallic;
                    }
                    sceneObjects.push_back(realObj);
                    continue;
                }
            }

            if (genIncludeLights)
            {
                SceneObject crystal;
                crystal.id = static_cast<int>(sceneObjects.size());
                crystal.name = "💎 Sky_Crystal_" + std::to_string(i);
                crystal.type = ObjectType::LIGHT;
                crystal.position = glm::vec3(rx, ry + 2.0f, rz);
                crystal.rotation = glm::vec3(0.0f, 45.0f, 45.0f);
                crystal.scale = glm::vec3(0.6f, 1.2f, 0.6f);
                crystal.color = glm::vec4(0.1f, 0.9f, 1.0f, 1.0f);
                crystal.meshId = primitiveCubeMeshId;

                auto lComp = std::make_shared<LightComponent>();
                lComp->color = crystal.color;
                lComp->intensity = 3.0f;
                crystal.components.push_back(lComp);
                sceneObjects.push_back(crystal);
            }
        }
    }
    else if (genPreset == 5) // 3D Maze
    {
        int mazeSize = 13;
        std::vector<std::vector<int>> maze(mazeSize, std::vector<int>(mazeSize, 1));

        std::function<void(int, int)> generateMazeDFS = [&](int cx, int cz) {
            maze[cz][cx] = 0;
            int dirs[4][2] = { {0, -2}, {2, 0}, {0, 2}, {-2, 0} };
            std::vector<int> p = {0, 1, 2, 3};
            std::shuffle(p.begin(), p.end(), rng);

            for (int i : p) {
                int nx = cx + dirs[i][0];
                int nz = cz + dirs[i][1];
                if (nx > 0 && nx < mazeSize - 1 && nz > 0 && nz < mazeSize - 1 && maze[nz][nx] == 1) {
                    maze[cz + dirs[i][1]/2][cx + dirs[i][0]/2] = 0;
                    generateMazeDFS(nx, nz);
                }
            }
        };

        generateMazeDFS(1, 1);

        float mCell = 2.0f;
        float offsetM = (mazeSize * mCell) * 0.5f;

        SceneObject floor;
        floor.id = static_cast<int>(sceneObjects.size());
        floor.name = "🌀 Maze_Floor";
        floor.type = ObjectType::PLANE;
        floor.position = glm::vec3(0.0f, 0.0f, 0.0f);
        floor.scale = glm::vec3(mazeSize * mCell, 1.0f, mazeSize * mCell);
        floor.color = glm::vec4(0.25f, 0.25f, 0.28f, 1.0f);
        floor.meshId = primitivePlaneMeshId;
        sceneObjects.push_back(floor);

        for (int z = 0; z < mazeSize; ++z)
        {
            for (int x = 0; x < mazeSize; ++x)
            {
                float wx = -offsetM + x * mCell + mCell * 0.5f;
                float wz = -offsetM + z * mCell + mCell * 0.5f;

                if (maze[z][x] == 1)
                {
                    SceneObject wall;
                    wall.id = static_cast<int>(sceneObjects.size());
                    wall.name = "🧱 Maze_Wall_" + std::to_string(x) + "_" + std::to_string(z);
                    wall.type = ObjectType::CUBE;
                    wall.position = glm::vec3(wx, 1.25f, wz);
                    wall.scale = glm::vec3(mCell, 2.5f, mCell);
                    wall.color = glm::vec4(0.42f, 0.42f, 0.46f, 1.0f);
                    wall.meshId = primitiveCubeMeshId;
                    sceneObjects.push_back(wall);
                }
                else if (genIncludeLights && dist01(rng) < 0.15f)
                {
                    if (genUseReal3DModels && !realModels.empty())
                    {
                        int lanternMeshId = assetManager->getOrLoadModelAsset("assets/models/Lantern.glb");
                        if (lanternMeshId >= 0)
                        {
                            SceneObject lantern;
                            lantern.id = static_cast<int>(sceneObjects.size());
                            lantern.name = "🏮 Maze_Lantern_" + std::to_string(x) + "_" + std::to_string(z);
                            lantern.type = ObjectType::LIGHT;
                            lantern.position = glm::vec3(wx, 0.5f, wz);
                            lantern.scale = glm::vec3(0.12f);
                            lantern.color = glm::vec4(1.0f, 0.7f, 0.3f, 1.0f);
                            lantern.meshId = lanternMeshId;
                            if (lanternMeshId < static_cast<int>(meshes.size())) {
                                lantern.textureId = meshes[lanternMeshId].defaultTextureId;
                                lantern.roughness = meshes[lanternMeshId].defaultRoughness;
                                lantern.metallic = meshes[lanternMeshId].defaultMetallic;
                            }

                            auto lComp = std::make_shared<LightComponent>();
                            lComp->color = lantern.color;
                            lComp->intensity = 2.5f;
                            lantern.components.push_back(lComp);
                            sceneObjects.push_back(lantern);
                            continue;
                        }
                    }

                    SceneObject torch;
                    torch.id = static_cast<int>(sceneObjects.size());
                    torch.name = "🔥 Torch_Light_" + std::to_string(x) + "_" + std::to_string(z);
                    torch.type = ObjectType::LIGHT;
                    torch.position = glm::vec3(wx, 1.8f, wz);
                    torch.scale = glm::vec3(0.2f);
                    torch.color = glm::vec4(1.0f, 0.55f, 0.1f, 1.0f);
                    torch.meshId = primitiveCubeMeshId;

                    auto lComp = std::make_shared<LightComponent>();
                    lComp->color = torch.color;
                    lComp->intensity = 2.0f;
                    torch.components.push_back(lComp);
                    sceneObjects.push_back(torch);
                }
            }
        }
    }

    printf("[3D Scene Generator] Created scene preset %d with %zu entities!\n", genPreset, sceneObjects.size());
}

void SceneGenerator::drawSceneGeneratorPanel(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem)
{
    if (!showSceneGeneratorPanel || !scene || !assetManager) return;

    ImGui::SetNextWindowSize(ImVec2(390, 620), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("🏞️ 3D Scene Generator (Sinh Cảnh 3D)", &showSceneGeneratorPanel))
    {
        ImGui::TextColored(ImVec4(0.2f, 0.85f, 1.0f, 1.0f), "✨ Sinh Cảnh 3D Tự Động (Procedural Scene)");
        ImGui::Separator();

        const char* presets[] = {
            "🌲 1. Rừng & Núi Đồi (Forest & Mountains)",
            "🏜️ 2. Sa Mạc & Cổ Tích (Desert Dunes & Ruins)",
            "🏙️ 3. Thành Phố Tương Lai (Cyberpunk City)",
            "🏰 4. Ngôi Làng & Lâu Đài (Medieval Citadel)",
            "🌌 5. Quần Đảo Phao (Floating Archipelago)",
            "🌀 6. Mê Cung 3D (3D Dungeon Maze)"
        };
        ImGui::Combo("Preset Mẫu", &genPreset, presets, IM_ARRAYSIZE(presets));

        ImGui::Spacing();
        ImGui::Checkbox("🌐 Sử Dụng Objects 3D Thực Tế (.GLB / .OBJ)", &genUseReal3DModels);

        ImGui::Spacing();
        ImGui::Text("⚙️ Thông Số Sinh Cảnh:");
        ImGui::InputInt("Seed (Hạt giống)", &genSeed);
        ImGui::SameLine();
        if (ImGui::Button("🎲 Random"))
        {
            genSeed = rand();
        }

        ImGui::SliderInt("Kích Thước Scene", &genGridSize, 10, 80);
        ImGui::SliderFloat("Độ Cao Địa Hình", &genHeightScale, 0.0f, 15.0f, "%.1f");
        ImGui::SliderFloat("Mật Độ Vật Thể", &genDensity, 0.1f, 3.0f, "%.2f");

        ImGui::Spacing();
        ImGui::Text("🎨 Phân Bố Thành Phần:");
        ImGui::Checkbox("Thảm Thực Vật / Cây (Trees)", &genIncludeTrees);
        ImGui::SameLine();
        ImGui::Checkbox("Đá / Mỏm Núi (Rocks)", &genIncludeRocks);
        ImGui::Checkbox("Tòa Nhà / Kiến Trúc (Buildings)", &genIncludeBuildings);
        ImGui::SameLine();
        ImGui::Checkbox("Ánh Sáng & Đèn (Lights)", &genIncludeLights);

        ImGui::Spacing();
        ImGui::Text("☀️ Chiếu Sáng Tự Nhiên:");
        ImGui::ColorEdit3("Màu Mặt Trời", &genSunColor.x);
        ImGui::SliderFloat("Cường Độ Nắng", &genSunIntensity, 0.5f, 5.0f, "%.1f");

        ImGui::Separator();
        ImGui::Checkbox("Xóa Cảnh Cũ Trước Khi Sinh", &genClearExisting);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));
        if (ImGui::Button(" 🎲 SINH CẢNH 3D MỚI (GENERATE SCENE) ", ImVec2(-1, 38)))
        {
            generate3DScene(scene, assetManager);
        }
        ImGui::PopStyleColor(2);

        // ------------------------------------------------------------
        // FIND & FOCUS MODEL IN SCENE (Like Blender / Unity / Maya)
        // ------------------------------------------------------------
        auto& sceneObjects = scene->getObjects();
        int selIdx = scene->getSelectedObjectIndex();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "🔍 Tìm & Focus Model Trong Cảnh (Find & Edit Models):");

        if (selIdx >= 0 && selIdx < static_cast<int>(sceneObjects.size()) && gizmoSystem)
        {
            std::string focusBtnLabel = " 🎯 Focus Vào Model Đang Chọn: " + sceneObjects[selIdx].name + " (Phím F) ";
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.45f, 0.1f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.55f, 0.2f, 1.0f));
            if (ImGui::Button(focusBtnLabel.c_str(), ImVec2(-1, 30)))
            {
                gizmoSystem->focusOnObject(selIdx, *scene);
            }
            ImGui::PopStyleColor(2);
        }

        static char searchFilter[64] = "";
        ImGui::InputTextWithHint("##SearchSceneObjs", "🔎 Tìm kiếm model trong cảnh...", searchFilter, sizeof(searchFilter));

        if (ImGui::TreeNodeEx("📋 Danh Sách & Tìm Model Đã Sinh Trong Cảnh", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (sceneObjects.empty())
            {
                ImGui::TextDisabled("Cảnh hiện tại chưa có vật thể nào. Bấm 'SINH CẢNH 3D MỚI' ở trên!");
            }
            else
            {
                std::string filterStr = searchFilter;
                std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                ImGui::BeginChild("GeneratedObjList", ImVec2(0, 180), true);
                for (size_t i = 0; i < sceneObjects.size(); ++i)
                {
                    std::string objName = sceneObjects[i].name;
                    std::string lowerName = objName;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                    if (!filterStr.empty() && lowerName.find(filterStr) == std::string::npos)
                    {
                        continue;
                    }

                    ImGui::PushID(static_cast<int>(i));
                    bool isSel = (selIdx == static_cast<int>(i));

                    std::string icon = "🧊 ";
                    if (sceneObjects[i].type == ObjectType::SPHERE) icon = "🟡 ";
                    else if (sceneObjects[i].type == ObjectType::PLANE) icon = "🟩 ";
                    else if (sceneObjects[i].type == ObjectType::LIGHT) icon = "☀️ ";
                    else if (lowerName.find("tree") != std::string::npos) icon = "🌲 ";
                    else if (lowerName.find("rock") != std::string::npos) icon = "🪨 ";
                    else if (lowerName.find("terrain") != std::string::npos) icon = "🏔️ ";
                    else if (lowerName.find("pyramid") != std::string::npos || lowerName.find("citadel") != std::string::npos) icon = "🏛️ ";

                    std::string itemLabel = icon + objName;
                    if (ImGui::Selectable(itemLabel.c_str(), isSel, 0, ImVec2(ImGui::GetContentRegionAvail().x - 75.0f, 0.0f)))
                    {
                        scene->setSelectedObjectIndex(static_cast<int>(i));
                        if (gizmoSystem) gizmoSystem->focusOnObject(static_cast<int>(i), *scene);
                    }

                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, isSel ? ImVec4(0.9f, 0.5f, 0.1f, 1.0f) : ImVec4(0.2f, 0.6f, 0.35f, 1.0f));
                    if (ImGui::SmallButton("🎯 Focus"))
                    {
                        scene->setSelectedObjectIndex(static_cast<int>(i));
                        if (gizmoSystem) gizmoSystem->focusOnObject(static_cast<int>(i), *scene);
                    }
                    ImGui::PopStyleColor();

                    ImGui::PopID();
                }
                ImGui::EndChild();
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();
        std::vector<std::string> detectedModels = scanAvailable3DModels();
        const auto& meshes = assetManager->getMeshes();

        if (ImGui::TreeNode("📦 Danh Sách Object 3D Đã Tải (.GLB / .OBJ)"))
        {
            if (detectedModels.empty())
            {
                ImGui::TextDisabled("Chưa tìm thấy file .glb hoặc .obj trong thư mục assets/models/");
            }
            else
            {
                for (const auto& mPath : detectedModels)
                {
                    std::filesystem::path p(mPath);
                    std::string mLabel = "🧊 " + p.filename().string();
                    if (ImGui::Selectable(mLabel.c_str()))
                    {
                        scene->saveHistory();
                        int mId = assetManager->getOrLoadModelAsset(mPath);
                        if (mId >= 0)
                        {
                            SceneObject newObj;
                            newObj.id = static_cast<int>(sceneObjects.size());
                            newObj.name = p.stem().string();
                            newObj.type = ObjectType::CUBE;
                            std::string stemName = p.stem().string();
                            float initScale = 1.0f;
                            if (stemName == "Fox") initScale = 0.02f;
                            else if (stemName == "Lantern") initScale = 0.2f;

                            if (stemName.find("rock") != std::string::npos || stemName.find("Rock") != std::string::npos)
                            {
                                newObj.scale = glm::vec3(0.35f, 0.10f, 0.35f);
                                newObj.rotation = glm::vec3(0.0f);
                                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);
                            }
                            else
                            {
                                newObj.scale = glm::vec3(initScale);
                                newObj.position = glm::vec3(0.0f, (initScale < 0.5f ? 0.0f : 0.5f), 0.0f);
                            }
                            newObj.color = glm::vec4(1.0f);
                            newObj.meshId = mId;
                            if (mId < static_cast<int>(meshes.size())) {
                                newObj.textureId = meshes[mId].defaultTextureId;
                                newObj.roughness = meshes[mId].defaultRoughness;
                                newObj.metallic = meshes[mId].defaultMetallic;
                            }
                            sceneObjects.push_back(newObj);
                            scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);
                        }
                    }
                }
            }
            ImGui::TreePop();
        }

        ImGui::Spacing();
        if (ImGui::Button("📂 Mở Thư Mục assets/models (Chứa File GLB/OBJ)", ImVec2(-1, 26)))
        {
            system("start assets\\models");
        }

        ImGui::Spacing();
        if (ImGui::Button("🧹 Xóa Tất Cả Vật Thể (Clear All)", ImVec2(-1, 26)))
        {
            scene->saveHistory();
            sceneObjects.clear();
            scene->setSelectedObjectIndex(-1);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Tổng số vật thể 3D hiện tại: %zu | Models 3D có sẵn: %zu", sceneObjects.size(), detectedModels.size());
    }
    ImGui::End();
}
