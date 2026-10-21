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
    std::vector<std::string> searchDirs = { "assets/models", "assets", "build/Release/assets/models" };
    std::vector<std::string> seenStems;
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
                        std::string lowerStem = stem;
                        for (auto& c : lowerStem) c = static_cast<char>(tolower(c));
                        if (lowerStem == "trees_and_bush_pack") lowerStem = "tree_and_bush";
                        if (std::find(seenStems.begin(), seenStems.end(), lowerStem) == seenStems.end())
                        {
                            seenStems.push_back(lowerStem);
                            modelFiles.push_back(entry.path().string());
                        }
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
      genHeightScale(2.5f),
      genDensity(1.0f),
      genIncludeTrees(true),
      genIncludeRocks(true),
      genIncludeBuildings(true),
      genIncludeLights(true),
      genClearExisting(true),
      genSunColor(1.0f, 0.95f, 0.85f),
      genSunIntensity(1.6f)
{
}

void SceneGenerator::generate3DScene(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem)
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
        sun.position = glm::vec3(6.0f, 18.0f, 12.0f);
        sun.rotation = glm::vec3(42.0f, 32.0f, 0.0f);
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
        if (genPreset == 0) { // Tactical Outpost & Riverbed Valley (scene_demo.jpg)
            float zRel = z - 2.0f;
            float gully = 1.0f - std::exp(-zRel * zRel * 0.08f);
            float fgSlope = (z > 2.0f) ? ((z - 2.0f) * 0.05f) : ((2.0f - z) * 0.10f);

            float mountains = 0.0f;
            if (z < -4.0f) {
                float distZ = (-4.0f - z) * 0.18f;
                mountains = (distZ * distZ * 0.45f) + std::sin(sx * 1.6f + 0.4f) * 0.55f + std::cos(sx * 2.8f) * 0.25f;
            }

            float lateral = (std::abs(x) > 4.5f) ? (std::abs(x) - 4.5f) * 0.06f : 0.0f;
            float micro = std::sin(sx * 1.4f + sz * 1.1f) * 0.12f + std::cos(sx * 3.2f - sz * 2.5f) * 0.06f;

            h = (fgSlope * gully + lateral + mountains + micro);
        } else if (genPreset == 1) { // Forest Clearing & Patrol (Sample_jungle.png Biotope 2)
            float fgSlope = (z > 2.0f) ? (z - 2.0f) * 0.04f : 0.0f;
            float mountains = 0.0f;
            if (z < -2.0f) {
                float distZ = (-2.0f - z) * 0.16f;
                mountains = distZ * distZ * 0.55f + std::sin(sx * 1.4f) * 0.65f + std::cos(sx * 2.6f + 0.8f) * 0.35f;
            }
            float lateral = (std::abs(x) > 4.5f) ? std::pow((std::abs(x) - 4.5f) * 0.12f, 1.4f) * 0.8f : 0.0f;
            float micro = std::sin(sx * 1.2f + sz * 1.5f) * 0.12f + std::cos(sx * 2.8f - sz * 2.1f) * 0.06f;
            h = (fgSlope + lateral + mountains + micro);
        } else if (genPreset == 2) { // Desert
            h += std::sin(sx * 0.7f + sz * 0.5f) * 0.8f;
            h += std::cos(sx * 1.8f - sz * 1.2f) * 0.2f;
        } else if (genPreset == 4) { // Medieval Valley
            float distFromCenter = std::sqrt(x * x + z * z);
            h = (1.0f - std::exp(-distFromCenter * 0.05f)) * 0.8f;
            h += std::sin(sx * 1.5f) * std::cos(sz * 1.5f) * 0.2f;
        } else if (genPreset == 5) { // Floating Islands
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

    auto setupRockTransform = [&](SceneObject& rockObj, float rx, float ry, float rz, float /*baseScale*/ = 1.0f) {
        float yaw = distRot(rng);
        rockObj.rotation = glm::vec3(0.0f, yaw, 0.0f);
        rockObj.scale = glm::vec3(1.0f);
        rockObj.position = glm::vec3(rx, ry, rz);
    };

    auto setupStoneTransform = [&](SceneObject& stoneObj, float rx, float ry, float rz, float /*baseScale*/ = 1.0f) {
        float yaw = distRot(rng);
        float pitch = (dist01(rng) * 2.0f - 1.0f) * 6.0f;
        float roll = (dist01(rng) * 2.0f - 1.0f) * 6.0f;
        stoneObj.rotation = glm::vec3(pitch, yaw, roll);
        stoneObj.scale = glm::vec3(1.0f);
        stoneObj.position = glm::vec3(rx, ry, rz);
    };

    auto setupBushTransform = [&](SceneObject& bushObj, float rx, float ry, float rz, float /*baseScale*/ = 1.0f) {
        float yaw = distRot(rng);
        bushObj.rotation = glm::vec3(0.0f, yaw, 0.0f);
        bushObj.scale = glm::vec3(1.0f);
        bushObj.position = glm::vec3(rx, ry, rz);
    };

    const auto& meshes = assetManager->getMeshes();

    if (genPreset == 0) // Tactical Outpost & Shooting Range (scene_demo.jpg)
    {
        // 1. Terrain Landscape (Riverbed Valley with Mountain backdrop)
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale, genSeed, 0);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🏜️ Outpost_Tactical_Terrain";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        terrain.color = glm::vec4(1.0f, 0.98f, 0.94f, 1.0f);
        terrain.texturePath = "assets/grass.jpg";
        terrain.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
        terrain.usePBR = true;
        terrain.roughness = 0.88f;
        terrain.metallic = 0.02f;
        sceneObjects.push_back(terrain);

        // 2. Left Observation Bunker Wall & Post (matches left foreground in scene_demo.jpg)
        if (genIncludeBuildings)
        {
            float wallX = -8.5f;
            float wallZ = 9.5f;
            float wallY = sampleHeight(wallX, wallZ);

            SceneObject bunkerWall;
            bunkerWall.id = static_cast<int>(sceneObjects.size());
            bunkerWall.name = "🏛️ Outpost_Bunker_Wall";
            bunkerWall.type = ObjectType::CUBE;
            bunkerWall.position = glm::vec3(wallX, wallY + 1.2f, wallZ);
            bunkerWall.rotation = glm::vec3(0.0f, 15.0f, 0.0f);
            bunkerWall.scale = glm::vec3(2.4f, 2.4f, 4.2f);
            bunkerWall.color = glm::vec4(0.76f, 0.68f, 0.54f, 1.0f);
            bunkerWall.meshId = primitiveCubeMeshId;
            bunkerWall.texturePath = "assets/grass.jpg";
            bunkerWall.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
            bunkerWall.usePBR = true;
            bunkerWall.roughness = 0.90f;
            sceneObjects.push_back(bunkerWall);

            SceneObject bunkerPillar;
            bunkerPillar.id = static_cast<int>(sceneObjects.size());
            bunkerPillar.name = "🏛️ Outpost_Bunker_Pillar";
            bunkerPillar.type = ObjectType::CUBE;
            bunkerPillar.position = glm::vec3(wallX + 1.1f, wallY + 1.5f, wallZ - 2.0f);
            bunkerPillar.rotation = glm::vec3(0.0f, 15.0f, 0.0f);
            bunkerPillar.scale = glm::vec3(1.2f, 3.0f, 1.2f);
            bunkerPillar.color = glm::vec4(0.72f, 0.64f, 0.50f, 1.0f);
            bunkerPillar.meshId = primitiveCubeMeshId;
            bunkerPillar.usePBR = true;
            bunkerPillar.roughness = 0.92f;
            sceneObjects.push_back(bunkerPillar);
        }

        // 3. Right Road Overpass / Concrete Bridge (matches right midground in scene_demo.jpg)
        if (genIncludeBuildings)
        {
            float brX = 9.5f;
            float brZ = 0.5f;
            float brY = sampleHeight(brX, brZ);

            SceneObject bridgeDeck;
            bridgeDeck.id = static_cast<int>(sceneObjects.size());
            bridgeDeck.name = "🌉 Outpost_Overpass_Bridge";
            bridgeDeck.type = ObjectType::CUBE;
            bridgeDeck.position = glm::vec3(brX, brY + 2.2f, brZ);
            bridgeDeck.rotation = glm::vec3(0.0f, -20.0f, 0.0f);
            bridgeDeck.scale = glm::vec3(3.4f, 0.5f, 8.5f);
            bridgeDeck.color = glm::vec4(0.72f, 0.72f, 0.70f, 1.0f);
            bridgeDeck.meshId = primitiveCubeMeshId;
            bridgeDeck.usePBR = true;
            bridgeDeck.roughness = 0.75f;
            sceneObjects.push_back(bridgeDeck);

            SceneObject pillar1;
            pillar1.id = static_cast<int>(sceneObjects.size());
            pillar1.name = "🏛️ Bridge_Pillar_1";
            pillar1.type = ObjectType::CUBE;
            pillar1.position = glm::vec3(brX - 0.5f, brY + 1.0f, brZ - 2.5f);
            pillar1.scale = glm::vec3(0.9f, 2.2f, 0.9f);
            pillar1.color = glm::vec4(0.68f, 0.68f, 0.66f, 1.0f);
            pillar1.meshId = primitiveCubeMeshId;
            pillar1.usePBR = true;
            pillar1.roughness = 0.80f;
            sceneObjects.push_back(pillar1);

            SceneObject pillar2;
            pillar2.id = static_cast<int>(sceneObjects.size());
            pillar2.name = "🏛️ Bridge_Pillar_2";
            pillar2.type = ObjectType::CUBE;
            pillar2.position = glm::vec3(brX + 0.5f, brY + 1.0f, brZ + 2.5f);
            pillar2.scale = glm::vec3(0.9f, 2.2f, 0.9f);
            pillar2.color = glm::vec4(0.68f, 0.68f, 0.66f, 1.0f);
            pillar2.meshId = primitiveCubeMeshId;
            pillar2.usePBR = true;
            pillar2.roughness = 0.80f;
            sceneObjects.push_back(pillar2);
        }

        // 4. Background Outpost Village & Water Tower (matches background in scene_demo.jpg)
        if (genIncludeBuildings)
        {
            float h1Y = sampleHeight(-3.2f, -11.0f);
            SceneObject b1;
            b1.id = static_cast<int>(sceneObjects.size());
            b1.name = "🏛️ Outpost_Building_1";
            b1.type = ObjectType::CUBE;
            b1.position = glm::vec3(-3.2f, h1Y + 1.1f, -11.0f);
            b1.rotation = glm::vec3(0.0f, 10.0f, 0.0f);
            b1.scale = glm::vec3(3.2f, 2.2f, 2.6f);
            b1.color = glm::vec4(0.80f, 0.72f, 0.60f, 1.0f);
            b1.meshId = primitiveCubeMeshId;
            b1.usePBR = true;
            b1.roughness = 0.88f;
            sceneObjects.push_back(b1);

            float h2Y = sampleHeight(1.5f, -12.5f);
            SceneObject b2;
            b2.id = static_cast<int>(sceneObjects.size());
            b2.name = "🏛️ Outpost_Building_2";
            b2.type = ObjectType::CUBE;
            b2.position = glm::vec3(1.5f, h2Y + 0.9f, -12.5f);
            b2.rotation = glm::vec3(0.0f, -15.0f, 0.0f);
            b2.scale = glm::vec3(2.6f, 1.8f, 2.2f);
            b2.color = glm::vec4(0.82f, 0.74f, 0.62f, 1.0f);
            b2.meshId = primitiveCubeMeshId;
            b2.usePBR = true;
            b2.roughness = 0.88f;
            sceneObjects.push_back(b2);

            float wY = sampleHeight(-0.8f, -9.8f);
            SceneObject cw;
            cw.id = static_cast<int>(sceneObjects.size());
            cw.name = "🏛️ Outpost_Compound_Wall";
            cw.type = ObjectType::CUBE;
            cw.position = glm::vec3(-0.8f, wY + 0.55f, -9.8f);
            cw.rotation = glm::vec3(0.0f, 5.0f, 0.0f);
            cw.scale = glm::vec3(4.8f, 1.1f, 0.4f);
            cw.color = glm::vec4(0.78f, 0.70f, 0.58f, 1.0f);
            cw.meshId = primitiveCubeMeshId;
            cw.usePBR = true;
            cw.roughness = 0.90f;
            sceneObjects.push_back(cw);

            float wtY = sampleHeight(-5.5f, -13.0f);
            SceneObject wtLeg;
            wtLeg.id = static_cast<int>(sceneObjects.size());
            wtLeg.name = "🗼 Water_Tower_Support";
            wtLeg.type = ObjectType::CUBE;
            wtLeg.position = glm::vec3(-5.5f, wtY + 1.8f, -13.0f);
            wtLeg.scale = glm::vec3(0.45f, 3.6f, 0.45f);
            wtLeg.color = glm::vec4(0.55f, 0.55f, 0.55f, 1.0f);
            wtLeg.meshId = (primitiveCylinderMeshId >= 0) ? primitiveCylinderMeshId : primitiveCubeMeshId;
            sceneObjects.push_back(wtLeg);

            SceneObject wtTank;
            wtTank.id = static_cast<int>(sceneObjects.size());
            wtTank.name = "🗼 Water_Tower_Tank";
            wtTank.type = ObjectType::CUBE;
            wtTank.position = glm::vec3(-5.5f, wtY + 3.9f, -13.0f);
            wtTank.scale = glm::vec3(1.6f, 1.4f, 1.6f);
            wtTank.color = glm::vec4(0.68f, 0.72f, 0.76f, 1.0f);
            wtTank.meshId = (primitiveCylinderMeshId >= 0) ? primitiveCylinderMeshId : primitiveSphereMeshId;
            sceneObjects.push_back(wtTank);
        }

        // 5. Tactical Humanoid Target in Midground (matches walking person in scene_demo.jpg)
        int cesiumId = assetManager->getOrLoadModelAsset("assets/models/CesiumMan.glb");
        if (cesiumId >= 0)
        {
            float targetX = 2.8f;
            float targetZ = 1.2f;
            float targetY = sampleHeight(targetX, targetZ);
            SceneObject targetObj;
            targetObj.id = static_cast<int>(sceneObjects.size());
            targetObj.name = "🎯 Tactical_Target_Operative";
            targetObj.type = ObjectType::CUBE;
            targetObj.position = glm::vec3(targetX, targetY, targetZ);
            targetObj.rotation = glm::vec3(0.0f, -40.0f, 0.0f);
            targetObj.scale = glm::vec3(1.0f);
            targetObj.color = glm::vec4(1.0f);
            targetObj.meshId = cesiumId;
            if (cesiumId < static_cast<int>(meshes.size())) {
                targetObj.textureId = meshes[cesiumId].defaultTextureId;
                targetObj.roughness = meshes[cesiumId].defaultRoughness;
                targetObj.metallic = meshes[cesiumId].defaultMetallic;
            }
            sceneObjects.push_back(targetObj);
        }

        // Secondary Tactical Target Relic
        int helmetId = assetManager->getOrLoadModelAsset("assets/models/DamagedHelmet.glb");
        if (helmetId >= 0)
        {
            float hX = -1.8f;
            float hZ = 2.5f;
            float hY = sampleHeight(hX, hZ);
            SceneObject helmetObj;
            helmetObj.id = static_cast<int>(sceneObjects.size());
            helmetObj.name = "🎯 Tactical_Target_Relic";
            helmetObj.type = ObjectType::CUBE;
            helmetObj.position = glm::vec3(hX, hY + 0.2f, hZ);
            helmetObj.scale = glm::vec3(1.0f);
            helmetObj.color = glm::vec4(1.0f);
            helmetObj.meshId = helmetId;
            if (helmetId < static_cast<int>(meshes.size())) {
                helmetObj.textureId = meshes[helmetId].defaultTextureId;
                helmetObj.roughness = meshes[helmetId].defaultRoughness;
                helmetObj.metallic = meshes[helmetId].defaultMetallic;
            }
            sceneObjects.push_back(helmetObj);
        }

        // 6. Savanna Trees & Palm Groves flanking Left & Right (matches trees in scene_demo.jpg)
        if (genIncludeTrees)
        {
            std::vector<std::string> treePaths = {
                "assets/models/tree_savanna.obj",
                "assets/models/tree_and_bush.glb",
                "assets/models/trees_and_bush_pack.glb",
                "assets/models/tree_cedar.obj",
                "assets/models/tree_pine.obj"
            };
            std::vector<std::string> validTrees;
            for (const auto& tp : treePaths) {
                if (std::filesystem::exists(tp)) validTrees.push_back(tp);
                else if (std::filesystem::exists("build/Release/" + tp)) validTrees.push_back("build/Release/" + tp);
            }

            // Left Flank Grove (X: -13 to -5.5, Z: -5 to 3)
            int leftCount = static_cast<int>(6 * genDensity);
            for (int k = 0; k < leftCount; ++k)
            {
                float tx = -5.5f - dist01(rng) * 7.5f;
                float tz = -5.0f + dist01(rng) * 8.0f;
                float ty = sampleHeight(tx, tz);
                if (!validTrees.empty())
                {
                    std::string chosenTree = validTrees[rng() % validTrees.size()];
                    int tId = assetManager->getOrLoadModelAsset(chosenTree);
                    if (tId >= 0)
                    {
                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌴 Left_Savanna_Tree_" + std::to_string(k);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(tx, ty, tz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(1.0f);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = tId;
                        if (tId < static_cast<int>(meshes.size())) {
                            treeObj.textureId = meshes[tId].defaultTextureId;
                            treeObj.roughness = meshes[tId].defaultRoughness;
                            treeObj.metallic = meshes[tId].defaultMetallic;
                        }
                        sceneObjects.push_back(treeObj);
                    }
                }
            }

            // Right Flank Grove (X: 5.5 to 13.5, Z: -6 to 2.5)
            int rightCount = static_cast<int>(6 * genDensity);
            for (int k = 0; k < rightCount; ++k)
            {
                float tx = 5.5f + dist01(rng) * 7.5f;
                float tz = -6.0f + dist01(rng) * 7.5f;
                // Avoid placing on bridge deck
                if (std::abs(tx - 9.5f) < 2.0f && std::abs(tz - 0.5f) < 3.0f) continue;
                float ty = sampleHeight(tx, tz);
                if (!validTrees.empty())
                {
                    std::string chosenTree = validTrees[rng() % validTrees.size()];
                    int tId = assetManager->getOrLoadModelAsset(chosenTree);
                    if (tId >= 0)
                    {
                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌴 Right_Savanna_Tree_" + std::to_string(k);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(tx, ty, tz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(1.0f);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = tId;
                        if (tId < static_cast<int>(meshes.size())) {
                            treeObj.textureId = meshes[tId].defaultTextureId;
                            treeObj.roughness = meshes[tId].defaultRoughness;
                            treeObj.metallic = meshes[tId].defaultMetallic;
                        }
                        sceneObjects.push_back(treeObj);
                    }
                }
            }

            // Background Village Grove
            int bgCount = static_cast<int>(4 * genDensity);
            for (int k = 0; k < bgCount; ++k)
            {
                float tx = (dist01(rng) * 2.0f - 1.0f) * 10.0f;
                float tz = -9.0f - dist01(rng) * 5.0f;
                if (std::abs(tx + 3.2f) < 2.5f && std::abs(tz + 11.0f) < 2.0f) continue;
                if (std::abs(tx - 1.5f) < 2.0f && std::abs(tz + 12.5f) < 2.0f) continue;
                float ty = sampleHeight(tx, tz);
                if (!validTrees.empty())
                {
                    std::string chosenTree = validTrees[rng() % validTrees.size()];
                    int tId = assetManager->getOrLoadModelAsset(chosenTree);
                    if (tId >= 0)
                    {
                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌴 Village_Palm_Tree_" + std::to_string(k);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(tx, ty, tz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(1.0f);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = tId;
                        if (tId < static_cast<int>(meshes.size())) {
                            treeObj.textureId = meshes[tId].defaultTextureId;
                            treeObj.roughness = meshes[tId].defaultRoughness;
                            treeObj.metallic = meshes[tId].defaultMetallic;
                        }
                        sceneObjects.push_back(treeObj);
                    }
                }
            }
        }

        // 7. Low Shrubs & Bushes (matches foreground & banks in scene_demo.jpg)
        std::string bushModelPath = "";
        if (std::filesystem::exists("assets/models/bush.glb")) bushModelPath = "assets/models/bush.glb";
        else if (std::filesystem::exists("build/Release/assets/models/bush.glb")) bushModelPath = "build/Release/assets/models/bush.glb";

        int bushMeshId = -1;
        if (!bushModelPath.empty()) bushMeshId = assetManager->getOrLoadModelAsset(bushModelPath);

        if (bushMeshId >= 0)
        {
            int bushCount = static_cast<int>(16 * genDensity);
            for (int b = 0; b < bushCount; ++b)
            {
                float bx = (dist01(rng) * 2.0f - 1.0f) * 12.0f;
                float bz = (dist01(rng) < 0.6f) ? (5.0f + dist01(rng) * 6.5f) : (dist01(rng) * 4.5f);
                if (bz > 12.0f && std::abs(bx) < 3.5f) continue; // Keep immediate camera lens clear

                float by = sampleHeight(bx, bz);
                SceneObject bushObj;
                bushObj.id = static_cast<int>(sceneObjects.size());
                bushObj.name = "🌿 Riverbank_Bush_" + std::to_string(b);
                bushObj.type = ObjectType::CUBE;
                setupBushTransform(bushObj, bx, by, bz);
                bushObj.color = glm::vec4(1.0f);
                bushObj.meshId = bushMeshId;
                if (bushMeshId < static_cast<int>(meshes.size())) {
                    bushObj.textureId = meshes[bushMeshId].defaultTextureId;
                    bushObj.roughness = meshes[bushMeshId].defaultRoughness;
                    bushObj.metallic = meshes[bushMeshId].defaultMetallic;
                }
                sceneObjects.push_back(bushObj);
            }
        }

        // 8. Natural Rocks & Stones (matches rocks along banks in scene_demo.jpg)
        if (genIncludeRocks)
        {
            std::string stonePath = "";
            if (std::filesystem::exists("assets/models/stone.glb")) stonePath = "assets/models/stone.glb";
            else if (std::filesystem::exists("build/Release/assets/models/stone.glb")) stonePath = "build/Release/assets/models/stone.glb";

            std::string rockPath = "";
            if (std::filesystem::exists("assets/models/rock.glb")) rockPath = "assets/models/rock.glb";
            else if (std::filesystem::exists("build/Release/assets/models/rock.glb")) rockPath = "build/Release/assets/models/rock.glb";

            int sId = (!stonePath.empty()) ? assetManager->getOrLoadModelAsset(stonePath) : -1;
            int rId = (!rockPath.empty()) ? assetManager->getOrLoadModelAsset(rockPath) : -1;

            int stoneCount = static_cast<int>(12 * genDensity);
            for (int s = 0; s < stoneCount; ++s)
            {
                float sx = (dist01(rng) * 2.0f - 1.0f) * 11.0f;
                float sz = -3.0f + dist01(rng) * 12.0f;
                if (sz > 12.0f && std::abs(sx) < 3.5f) continue; // Avoid camera lens

                float sy = sampleHeight(sx, sz);
                bool useBoulder = (s % 3 == 0 && rId >= 0);
                int chosenMeshId = useBoulder ? rId : sId;
                if (chosenMeshId >= 0)
                {
                    SceneObject rockObj;
                    rockObj.id = static_cast<int>(sceneObjects.size());
                    rockObj.name = useBoulder ? ("🪨 Riverbank_Boulder_" + std::to_string(s)) : ("🪨 Riverbank_Stone_" + std::to_string(s));
                    rockObj.type = ObjectType::CUBE;
                    if (useBoulder) {
                        setupRockTransform(rockObj, sx, sy, sz);
                    } else {
                        setupStoneTransform(rockObj, sx, sy, sz);
                    }
                    rockObj.color = glm::vec4(1.0f);
                    rockObj.meshId = chosenMeshId;
                    if (chosenMeshId < static_cast<int>(meshes.size())) {
                        rockObj.textureId = meshes[chosenMeshId].defaultTextureId;
                        rockObj.roughness = meshes[chosenMeshId].defaultRoughness;
                        rockObj.metallic = meshes[chosenMeshId].defaultMetallic;
                    }
                    sceneObjects.push_back(rockObj);
                }
            }
        }
    }
    else if (genPreset == 1) // Forest Clearing & Patrol (Sample_jungle.png Biotope 2)
    {
        // 1. Terrain Landscape (Clearing with rolling green hills and mountains behind)
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale, genSeed, 5);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🌲 Forest_Clearing_Terrain";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        terrain.color = glm::vec4(0.92f, 1.0f, 0.90f, 1.0f);
        terrain.texturePath = "assets/grass.jpg";
        terrain.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
        terrain.usePBR = true;
        terrain.roughness = 0.85f;
        terrain.metallic = 0.02f;
        sceneObjects.push_back(terrain);

        // 2. Fallen Tree Logs & Log Piles (Using real 3D assets: log_pile.glb and tree_log.glb)
        std::string logPilePath = "";
        if (std::filesystem::exists("assets/models/log_pile.glb")) logPilePath = "assets/models/log_pile.glb";
        else if (std::filesystem::exists("build/Release/assets/models/log_pile.glb")) logPilePath = "build/Release/assets/models/log_pile.glb";

        std::string treeLogPath = "";
        if (std::filesystem::exists("assets/models/tree_log.glb")) treeLogPath = "assets/models/tree_log.glb";
        else if (std::filesystem::exists("build/Release/assets/models/tree_log.glb")) treeLogPath = "build/Release/assets/models/tree_log.glb";

        int logPileMeshId = (!logPilePath.empty()) ? assetManager->getOrLoadModelAsset(logPilePath) : -1;
        int treeLogMeshId = (!treeLogPath.empty()) ? assetManager->getOrLoadModelAsset(treeLogPath) : -1;

        // Place stacked log piles (log_pile.glb - ~3.1m x 0.58m x 1.1m)
        if (logPileMeshId >= 0)
        {
            struct LogPileDef { float x, z, yaw; const char* name; };
            std::vector<LogPileDef> pileDefs = {
                { -3.6f, 7.2f,  18.0f, "🪵 Forest_Log_Pile_1" },
                {  4.2f, 5.8f, -28.0f, "🪵 Clearing_Log_Pile_2" },
                { -1.8f, 0.8f,  75.0f, "🪵 Woodcutter_Log_Pile_3" },
                {  5.5f, 9.6f, -10.0f, "🪵 Reserve_Log_Pile_4" }
            };

            for (const auto& p : pileDefs)
            {
                float py = sampleHeight(p.x, p.z);
                SceneObject pileObj;
                pileObj.id = static_cast<int>(sceneObjects.size());
                pileObj.name = p.name;
                pileObj.type = ObjectType::CUBE;
                pileObj.position = glm::vec3(p.x, py, p.z);
                pileObj.rotation = glm::vec3(0.0f, p.yaw, 0.0f);
                pileObj.scale = glm::vec3(1.0f);
                pileObj.color = glm::vec4(1.0f);
                pileObj.meshId = logPileMeshId;
                if (logPileMeshId < static_cast<int>(meshes.size())) {
                    pileObj.textureId = meshes[logPileMeshId].defaultTextureId;
                    pileObj.roughness = meshes[logPileMeshId].defaultRoughness;
                    pileObj.metallic = meshes[logPileMeshId].defaultMetallic;
                }
                sceneObjects.push_back(pileObj);
            }
        }

        // Place individual fallen tree logs (tree_log.glb - ~1.79m x 0.38m x 0.62m)
        if (treeLogMeshId >= 0)
        {
            struct TreeLogDef { float x, z, yaw; const char* name; };
            std::vector<TreeLogDef> logDefs = {
                { -0.8f,  2.2f,  62.0f, "🪵 Fallen_Tree_Log_1" },
                {  2.5f,  3.8f, -32.0f, "🪵 Fallen_Tree_Log_2" },
                { -2.2f,  4.6f,  45.0f, "🪵 Tactical_Fallen_Log_3" },
                { -4.2f,  3.2f, -15.0f, "🪵 Mossy_Tree_Log_4" },
                {  4.5f,  8.2f, -70.0f, "🪵 Forest_Tree_Log_5" },
                {  1.2f, -1.0f,  10.0f, "🪵 Ridge_Tree_Log_6" }
            };

            for (const auto& l : logDefs)
            {
                float ly = sampleHeight(l.x, l.z);
                SceneObject lObj;
                lObj.id = static_cast<int>(sceneObjects.size());
                lObj.name = l.name;
                lObj.type = ObjectType::CUBE;
                lObj.position = glm::vec3(l.x, ly, l.z);
                lObj.rotation = glm::vec3(0.0f, l.yaw, 0.0f);
                lObj.scale = glm::vec3(1.0f);
                lObj.color = glm::vec4(1.0f);
                lObj.meshId = treeLogMeshId;
                if (treeLogMeshId < static_cast<int>(meshes.size())) {
                    lObj.textureId = meshes[treeLogMeshId].defaultTextureId;
                    lObj.roughness = meshes[treeLogMeshId].defaultRoughness;
                    lObj.metallic = meshes[treeLogMeshId].defaultMetallic;
                }
                sceneObjects.push_back(lObj);
            }
        }
        else if (logPileMeshId < 0)
        {
            // Procedural fallback if GLB files are unavailable
            struct LogDef { float x, z, yaw, length, radius; const char* name; };
            std::vector<LogDef> fallenLogs = {
                { -3.2f, 7.5f,  25.0f, 3.4f, 0.35f, "🪵 Fallen_Tree_Trunk_1" },
                {  3.5f, 5.2f, -32.0f, 2.8f, 0.32f, "🪵 Fallen_Tree_Trunk_2" },
                { -0.6f, 1.8f,  68.0f, 2.4f, 0.28f, "🪵 Fallen_Tree_Trunk_3" }
            };

            for (const auto& log : fallenLogs)
            {
                float ly = sampleHeight(log.x, log.z) + log.radius * 0.85f;
                SceneObject logObj;
                logObj.id = static_cast<int>(sceneObjects.size());
                logObj.name = log.name;
                logObj.type = ObjectType::CUBE;
                logObj.position = glm::vec3(log.x, ly, log.z);
                logObj.rotation = glm::vec3(0.0f, log.yaw, 90.0f);
                logObj.scale = glm::vec3(log.radius * 2.0f, log.length, log.radius * 2.0f);
                logObj.color = glm::vec4(0.68f, 0.58f, 0.46f, 1.0f);
                logObj.meshId = (primitiveCylinderMeshId >= 0) ? primitiveCylinderMeshId : primitiveCubeMeshId;
                logObj.texturePath = "assets/Texture/Bark___0.jpg";
                logObj.textureId = assetManager->getOrLoadTextureAsset("assets/Texture/Bark___0.jpg");
                logObj.usePBR = true;
                logObj.roughness = 0.90f;
                sceneObjects.push_back(logObj);
            }
        }

        // 3. Outdoor Tactical Shooting Range Targets in the Clearing (Replaces helmet & human)
        std::string shootTargetObj = "";
        if (std::filesystem::exists("assets/models/ShootingTarget.obj")) shootTargetObj = "assets/models/ShootingTarget.obj";
        else if (std::filesystem::exists("build/Release/assets/models/ShootingTarget.obj")) shootTargetObj = "build/Release/assets/models/ShootingTarget.obj";
        else if (std::filesystem::exists("assets/models/ShootingTarget.fbx")) shootTargetObj = "assets/models/ShootingTarget.fbx";
        int shootTargetMeshId = (!shootTargetObj.empty()) ? assetManager->getOrLoadModelAsset(shootTargetObj) : -1;

        std::string ipscPath = "";
        if (std::filesystem::exists("assets/models/IPSC_Metric_Target.obj")) ipscPath = "assets/models/IPSC_Metric_Target.obj";
        else if (std::filesystem::exists("build/Release/assets/models/IPSC_Metric_Target.obj")) ipscPath = "build/Release/assets/models/IPSC_Metric_Target.obj";
        int ipscMeshId = (!ipscPath.empty()) ? assetManager->getOrLoadModelAsset(ipscPath) : -1;

        std::string popperPath = "";
        if (std::filesystem::exists("assets/models/Pepper_Popper.obj")) popperPath = "assets/models/Pepper_Popper.obj";
        else if (std::filesystem::exists("build/Release/assets/models/Pepper_Popper.obj")) popperPath = "build/Release/assets/models/Pepper_Popper.obj";
        int popperMeshId = (!popperPath.empty()) ? assetManager->getOrLoadModelAsset(popperPath) : -1;

        std::string platePath = "";
        if (std::filesystem::exists("assets/models/Plate_A.obj")) platePath = "assets/models/Plate_A.obj";
        else if (std::filesystem::exists("build/Release/assets/models/Plate_A.obj")) platePath = "build/Release/assets/models/Plate_A.obj";
        int plateMeshId = (!platePath.empty()) ? assetManager->getOrLoadModelAsset(platePath) : -1;

        std::string barricadePath = "";
        if (std::filesystem::exists("assets/models/Wall_Barricade.obj")) barricadePath = "assets/models/Wall_Barricade.obj";
        else if (std::filesystem::exists("build/Release/assets/models/Wall_Barricade.obj")) barricadePath = "build/Release/assets/models/Wall_Barricade.obj";
        int barricadeMeshId = (!barricadePath.empty()) ? assetManager->getOrLoadModelAsset(barricadePath) : -1;

        auto placeShootingTarget = [&](int meshId, const std::string& path, const char* name, float x, float z, float rotY, const glm::vec4& color = glm::vec4(1.0f), bool isMetallic = false) {
            if (meshId < 0) return;
            float ty = sampleHeight(x, z);
            SceneObject tObj;
            tObj.id = static_cast<int>(sceneObjects.size());
            tObj.name = name;
            tObj.type = ObjectType::CUBE;
            tObj.position = glm::vec3(x, ty, z);
            tObj.rotation = glm::vec3(0.0f, rotY, 0.0f);
            tObj.scale = glm::vec3(1.0f);
            tObj.color = color;
            tObj.meshId = meshId;
            tObj.modelPath = path;
            tObj.usePBR = true;
            if (meshId < static_cast<int>(meshes.size())) {
                tObj.textureId = meshes[meshId].defaultTextureId;
                tObj.roughness = isMetallic ? 0.35f : meshes[meshId].defaultRoughness;
                tObj.metallic = isMetallic ? 0.85f : meshes[meshId].defaultMetallic;
                if (tObj.textureId >= 0 && assetManager) {
                    tObj.texturePath = assetManager->getTexturePath(tObj.textureId);
                }
            }
            if (tObj.textureId < 0 && (path.find("ShootingTarget") != std::string::npos)) {
                tObj.texturePath = "assets/models/ShootingTarget.png";
                tObj.textureId = assetManager->getOrLoadTextureAsset("assets/models/ShootingTarget.png");
            }
            sceneObjects.push_back(tObj);
        };

        // 3.1 Main Shooting Target Stands (ShootingTarget.obj)
        placeShootingTarget(shootTargetMeshId, shootTargetObj, "🎯 Forest_ShootingTarget_Center", 0.0f, 3.5f, 180.0f);
        placeShootingTarget(shootTargetMeshId, shootTargetObj, "🎯 Forest_ShootingTarget_LeftFlank", -2.8f, 5.0f, 168.0f);
        placeShootingTarget(shootTargetMeshId, shootTargetObj, "🎯 Forest_ShootingTarget_RightFlank", 2.8f, 4.5f, 192.0f);

        // 3.2 IPSC Cardboard Metric Targets (IPSC_Metric_Target.obj)
        placeShootingTarget(ipscMeshId, ipscPath, "🎯 Forest_IPSC_Target_NearLeft", -1.6f, 6.5f, 175.0f);
        placeShootingTarget(ipscMeshId, ipscPath, "🎯 Forest_IPSC_Target_NearRight", 1.8f, 6.8f, 185.0f);

        // 3.3 Steel Pepper Poppers (Pepper_Popper.obj)
        placeShootingTarget(popperMeshId, popperPath, "🔘 Steel_Pepper_Popper_Left", -3.6f, 2.0f, 165.0f, glm::vec4(0.92f, 0.95f, 0.98f, 1.0f), true);
        placeShootingTarget(popperMeshId, popperPath, "🔘 Steel_Pepper_Popper_Right", 3.5f, 1.8f, 195.0f, glm::vec4(0.92f, 0.95f, 0.98f, 1.0f), true);

        // 3.4 Steel Round Plates on Stands (Plate_A.obj)
        placeShootingTarget(plateMeshId, platePath, "🔘 Steel_Plate_Target_MidLeft", -0.9f, 1.2f, 180.0f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), true);
        placeShootingTarget(plateMeshId, platePath, "🔘 Steel_Plate_Target_MidRight", 0.9f, 1.2f, 180.0f, glm::vec4(0.95f, 0.95f, 0.95f, 1.0f), true);

        // 3.5 Tactical Wooden Shooting Barricade (Wall_Barricade.obj)
        placeShootingTarget(barricadeMeshId, barricadePath, "🛡️ Tactical_Shooting_Barricade", -1.8f, 9.2f, 15.0f);

        // 4. Forest Trees (Left flank, Right flank, and Rear mountain ridge tree line)
        if (genIncludeTrees)
        {
            std::vector<std::string> treePaths = {
                "assets/models/tree_pine.obj",
                "assets/models/tree_cedar.obj",
                "assets/models/tree_mossy.obj",
                "assets/models/tree_oak.obj",
                "assets/models/tree_redwood.obj",
                "assets/models/tree9.obj",
                "assets/models/tree_and_bush.glb",
                "assets/models/trees_and_bush_pack.glb"
            };
            std::vector<std::string> validTrees;
            for (const auto& tp : treePaths) {
                if (std::filesystem::exists(tp)) validTrees.push_back(tp);
                else if (std::filesystem::exists("build/Release/" + tp)) validTrees.push_back("build/Release/" + tp);
            }

            // Left Forest Flank (X: -13 to -4.5, Z: -5 to 11)
            int leftCount = static_cast<int>(12 * genDensity);
            for (int k = 0; k < leftCount; ++k)
            {
                float tx = -4.5f - dist01(rng) * 8.5f;
                float tz = -5.0f + dist01(rng) * 16.0f;
                float ty = sampleHeight(tx, tz);
                if (!validTrees.empty())
                {
                    std::string chosenTree = validTrees[rng() % validTrees.size()];
                    int tId = assetManager->getOrLoadModelAsset(chosenTree);
                    if (tId >= 0)
                    {
                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌲 Left_Forest_Tree_" + std::to_string(k);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(tx, ty, tz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(1.0f);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = tId;
                        if (tId < static_cast<int>(meshes.size())) {
                            treeObj.textureId = meshes[tId].defaultTextureId;
                            treeObj.roughness = meshes[tId].defaultRoughness;
                            treeObj.metallic = meshes[tId].defaultMetallic;
                        }
                        sceneObjects.push_back(treeObj);
                    }
                }
            }

            // Right Forest Flank (X: 4.5 to 13, Z: -5 to 11)
            int rightCount = static_cast<int>(12 * genDensity);
            for (int k = 0; k < rightCount; ++k)
            {
                float tx = 4.5f + dist01(rng) * 8.5f;
                float tz = -5.0f + dist01(rng) * 16.0f;
                float ty = sampleHeight(tx, tz);
                if (!validTrees.empty())
                {
                    std::string chosenTree = validTrees[rng() % validTrees.size()];
                    int tId = assetManager->getOrLoadModelAsset(chosenTree);
                    if (tId >= 0)
                    {
                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌲 Right_Forest_Tree_" + std::to_string(k);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(tx, ty, tz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(1.0f);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = tId;
                        if (tId < static_cast<int>(meshes.size())) {
                            treeObj.textureId = meshes[tId].defaultTextureId;
                            treeObj.roughness = meshes[tId].defaultRoughness;
                            treeObj.metallic = meshes[tId].defaultMetallic;
                        }
                        sceneObjects.push_back(treeObj);
                    }
                }
            }

            // Rear Tree Line on Rising Mountain Slopes (Z: -12 to -3)
            int rearCount = static_cast<int>(14 * genDensity);
            for (int k = 0; k < rearCount; ++k)
            {
                float tx = (dist01(rng) * 2.0f - 1.0f) * 12.0f;
                float tz = -3.5f - dist01(rng) * 9.5f;
                float ty = sampleHeight(tx, tz);
                if (!validTrees.empty())
                {
                    std::string chosenTree = validTrees[rng() % validTrees.size()];
                    int tId = assetManager->getOrLoadModelAsset(chosenTree);
                    if (tId >= 0)
                    {
                        SceneObject treeObj;
                        treeObj.id = static_cast<int>(sceneObjects.size());
                        treeObj.name = "🌲 Mountain_Ridge_Tree_" + std::to_string(k);
                        treeObj.type = ObjectType::CUBE;
                        treeObj.position = glm::vec3(tx, ty, tz);
                        treeObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        treeObj.scale = glm::vec3(1.0f);
                        treeObj.color = glm::vec4(1.0f);
                        treeObj.meshId = tId;
                        if (tId < static_cast<int>(meshes.size())) {
                            treeObj.textureId = meshes[tId].defaultTextureId;
                            treeObj.roughness = meshes[tId].defaultRoughness;
                            treeObj.metallic = meshes[tId].defaultMetallic;
                        }
                        sceneObjects.push_back(treeObj);
                    }
                }
            }
        }

        // 5. Lush Meadow Bushes & Shrubbery (dotted across the clearing)
        std::string bushModelPath = "";
        if (std::filesystem::exists("assets/models/bush.glb")) bushModelPath = "assets/models/bush.glb";
        else if (std::filesystem::exists("build/Release/assets/models/bush.glb")) bushModelPath = "build/Release/assets/models/bush.glb";

        int bushMeshId = (!bushModelPath.empty()) ? assetManager->getOrLoadModelAsset(bushModelPath) : -1;
        if (bushMeshId >= 0)
        {
            int bushCount = static_cast<int>(20 * genDensity);
            for (int b = 0; b < bushCount; ++b)
            {
                float bx = (dist01(rng) * 2.0f - 1.0f) * 11.0f;
                float bz = -2.0f + dist01(rng) * 13.0f;
                if (bz > 12.0f && std::abs(bx) < 3.5f) continue; // Keep camera lens clear

                float by = sampleHeight(bx, bz);
                SceneObject bushObj;
                bushObj.id = static_cast<int>(sceneObjects.size());
                bushObj.name = "🌿 Meadow_Bush_" + std::to_string(b);
                bushObj.type = ObjectType::CUBE;
                setupBushTransform(bushObj, bx, by, bz);
                bushObj.color = glm::vec4(1.0f);
                bushObj.meshId = bushMeshId;
                if (bushMeshId < static_cast<int>(meshes.size())) {
                    bushObj.textureId = meshes[bushMeshId].defaultTextureId;
                    bushObj.roughness = meshes[bushMeshId].defaultRoughness;
                    bushObj.metallic = meshes[bushMeshId].defaultMetallic;
                }
                sceneObjects.push_back(bushObj);
            }
        }

        // 6. Natural Field Stones & Rocks (weathered rocks in grass)
        if (genIncludeRocks)
        {
            std::string stonePath = "";
            if (std::filesystem::exists("assets/models/stone.glb")) stonePath = "assets/models/stone.glb";
            else if (std::filesystem::exists("build/Release/assets/models/stone.glb")) stonePath = "build/Release/assets/models/stone.glb";

            std::string rockPath = "";
            if (std::filesystem::exists("assets/models/rock.glb")) rockPath = "assets/models/rock.glb";
            else if (std::filesystem::exists("build/Release/assets/models/rock.glb")) rockPath = "build/Release/assets/models/rock.glb";

            int sId = (!stonePath.empty()) ? assetManager->getOrLoadModelAsset(stonePath) : -1;
            int rId = (!rockPath.empty()) ? assetManager->getOrLoadModelAsset(rockPath) : -1;

            int stoneCount = static_cast<int>(14 * genDensity);
            for (int s = 0; s < stoneCount; ++s)
            {
                float sx = (dist01(rng) * 2.0f - 1.0f) * 11.5f;
                float sz = -3.0f + dist01(rng) * 14.0f;
                if (sz > 12.0f && std::abs(sx) < 3.5f) continue;

                float sy = sampleHeight(sx, sz);
                bool useBoulder = (s % 3 == 0 && rId >= 0);
                int chosenMeshId = useBoulder ? rId : sId;
                if (chosenMeshId >= 0)
                {
                    SceneObject rockObj;
                    rockObj.id = static_cast<int>(sceneObjects.size());
                    rockObj.name = useBoulder ? ("🪨 Meadow_Boulder_" + std::to_string(s)) : ("🪨 Meadow_Stone_" + std::to_string(s));
                    rockObj.type = ObjectType::CUBE;
                    if (useBoulder) {
                        setupRockTransform(rockObj, sx, sy, sz);
                    } else {
                        setupStoneTransform(rockObj, sx, sy, sz);
                    }
                    rockObj.color = glm::vec4(1.0f);
                    rockObj.meshId = chosenMeshId;
                    if (chosenMeshId < static_cast<int>(meshes.size())) {
                        rockObj.textureId = meshes[chosenMeshId].defaultTextureId;
                        rockObj.roughness = meshes[chosenMeshId].defaultRoughness;
                        rockObj.metallic = meshes[chosenMeshId].defaultMetallic;
                    }
                    sceneObjects.push_back(rockObj);
                }
            }
        }
    }
    else if (genPreset == 2) // Desert
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale * 0.6f, genSeed, 1);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🏜️ Desert_Dunes";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        terrain.color = glm::vec4(1.0f);
        terrain.texturePath = "assets/grass.jpg";
        terrain.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
        terrain.usePBR = true;
        terrain.roughness = 0.90f;
        terrain.metallic = 0.0f;
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
                    relic.scale = glm::vec3(1.0f);
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
                    std::string lowerMName = mName;
                    for (auto& c : lowerMName) c = static_cast<char>(tolower(c));

                    if (lowerMName.find("stone") != std::string::npos)
                    {
                        glm::vec3 n = sampleNormal(rx, rz);
                        if (n.y < 0.85f) continue;
                        setupStoneTransform(realObj, rx, ry, rz);
                    }
                    else if (lowerMName.find("rock") != std::string::npos)
                    {
                        glm::vec3 n = sampleNormal(rx, rz);
                        if (n.y < 0.88f) continue;
                        setupRockTransform(realObj, rx, ry, rz);
                    }
                    else if (lowerMName.find("bush") != std::string::npos && lowerMName.find("tree") == std::string::npos)
                    {
                        setupBushTransform(realObj, rx, ry, rz);
                    }
                    else if (lowerMName.find("tree_and_bush") != std::string::npos || lowerMName.find("trees_and_bush") != std::string::npos)
                    {
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        realObj.position = glm::vec3(rx, ry, rz);
                        realObj.scale = glm::vec3(1.0f);
                    }
                    else
                    {
                        bool snapGround = (lowerMName.find("tree") != std::string::npos || lowerMName.find("bush") != std::string::npos || lowerMName.find("log") != std::string::npos);
                        realObj.position = glm::vec3(rx, snapGround ? ry : (ry + 0.5f), rz);
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        realObj.scale = glm::vec3(1.0f);
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
    else if (genPreset == 3) // Cyberpunk City
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, 0.0f, genSeed, 2);
        SceneObject ground;
        ground.id = static_cast<int>(sceneObjects.size());
        ground.name = "🌃 City_Ground_Grid";
        ground.type = ObjectType::PLANE;
        ground.position = glm::vec3(0.0f, 0.0f, 0.0f);
        ground.meshId = terrainMeshId;
        ground.color = glm::vec4(1.0f);
        ground.texturePath = "assets/road.jpg";
        ground.textureId = assetManager->getOrLoadTextureAsset("assets/road.jpg");
        ground.usePBR = true;
        ground.roughness = 0.70f;
        ground.metallic = 0.1f;
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
    else if (genPreset == 4) // Medieval Village
    {
        int terrainMeshId = assetManager->createTerrainMesh(genGridSize, genGridSize, cellSize, genHeightScale * 0.4f, genSeed, 3);
        SceneObject terrain;
        terrain.id = static_cast<int>(sceneObjects.size());
        terrain.name = "🏰 Citadel_Valley";
        terrain.type = ObjectType::PLANE;
        terrain.position = glm::vec3(0.0f, 0.0f, 0.0f);
        terrain.meshId = terrainMeshId;
        terrain.color = glm::vec4(1.0f);
        terrain.texturePath = "assets/grass.jpg";
        terrain.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
        terrain.usePBR = true;
        terrain.roughness = 0.85f;
        terrain.metallic = 0.05f;
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

        if (genIncludeTrees || genIncludeRocks)
        {
            int natureCount = static_cast<int>(16 * genDensity);
            for (int k = 0; k < natureCount; ++k)
            {
                float angle = distRot(rng) * 0.0174533f;
                float radius = 5.0f + dist01(rng) * 12.0f;
                float rx = std::cos(angle) * radius;
                float rz = std::sin(angle) * radius;

                // Keep main camera view corridor clear
                float camDistSq = rx * rx + (rz - 14.5f) * (rz - 14.5f);
                if (camDistSq < 20.0f) continue;

                float ry = sampleHeight(rx, rz);

                if (genIncludeRocks && dist01(rng) < 0.40f)
                {
                    std::string stonePath = "";
                    if (std::filesystem::exists("assets/models/stone.glb")) stonePath = "assets/models/stone.glb";
                    else if (std::filesystem::exists("build/Release/assets/models/stone.glb")) stonePath = "build/Release/assets/models/stone.glb";
                    if (stonePath.empty()) {
                        if (std::filesystem::exists("assets/models/rock.glb")) stonePath = "assets/models/rock.glb";
                        else if (std::filesystem::exists("build/Release/assets/models/rock.glb")) stonePath = "build/Release/assets/models/rock.glb";
                    }

                    if (!stonePath.empty())
                    {
                        int sMeshId = assetManager->getOrLoadModelAsset(stonePath);
                        if (sMeshId >= 0)
                        {
                            SceneObject stoneObj;
                            stoneObj.id = static_cast<int>(sceneObjects.size());
                            stoneObj.type = ObjectType::CUBE;
                            stoneObj.name = (stonePath.find("stone") != std::string::npos) ? ("🪨 Citadel_Stone_" + std::to_string(k)) : ("🪨 Citadel_Rock_" + std::to_string(k));
                            setupStoneTransform(stoneObj, rx, ry, rz);
                            stoneObj.color = glm::vec4(1.0f);
                            stoneObj.meshId = sMeshId;
                            if (sMeshId < static_cast<int>(meshes.size())) {
                                stoneObj.textureId = meshes[sMeshId].defaultTextureId;
                                stoneObj.roughness = meshes[sMeshId].defaultRoughness;
                                stoneObj.metallic = meshes[sMeshId].defaultMetallic;
                            }
                            sceneObjects.push_back(stoneObj);
                            continue;
                        }
                    }
                }

                if (genIncludeTrees)
                {
                    if (dist01(rng) < 0.50f)
                    {
                        std::string bushPath = "";
                        if (std::filesystem::exists("assets/models/bush.glb")) bushPath = "assets/models/bush.glb";
                        else if (std::filesystem::exists("build/Release/assets/models/bush.glb")) bushPath = "build/Release/assets/models/bush.glb";

                        if (!bushPath.empty())
                        {
                            int bMeshId = assetManager->getOrLoadModelAsset(bushPath);
                            if (bMeshId >= 0)
                            {
                                SceneObject bObj;
                                bObj.id = static_cast<int>(sceneObjects.size());
                                bObj.name = "🌿 Village_Bush_" + std::to_string(k);
                                bObj.type = ObjectType::CUBE;
                                setupBushTransform(bObj, rx, ry, rz);
                                bObj.color = glm::vec4(1.0f);
                                bObj.meshId = bMeshId;
                                if (bMeshId < static_cast<int>(meshes.size())) {
                                    bObj.textureId = meshes[bMeshId].defaultTextureId;
                                    bObj.roughness = meshes[bMeshId].defaultRoughness;
                                    bObj.metallic = meshes[bMeshId].defaultMetallic;
                                }
                                sceneObjects.push_back(bObj);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        std::string clusterPath = "";
                        if (std::filesystem::exists("assets/models/tree_and_bush.glb")) clusterPath = "assets/models/tree_and_bush.glb";
                        else if (std::filesystem::exists("assets/models/trees_and_bush_pack.glb")) clusterPath = "assets/models/trees_and_bush_pack.glb";
                        else if (std::filesystem::exists("build/Release/assets/models/tree_and_bush.glb")) clusterPath = "build/Release/assets/models/tree_and_bush.glb";

                        if (!clusterPath.empty())
                        {
                            int clMeshId = assetManager->getOrLoadModelAsset(clusterPath);
                            if (clMeshId >= 0)
                            {
                                SceneObject clObj;
                                clObj.id = static_cast<int>(sceneObjects.size());
                                clObj.name = "🌳 Village_Grove_" + std::to_string(k);
                                clObj.type = ObjectType::CUBE;
                                clObj.position = glm::vec3(rx, ry, rz);
                                clObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                                clObj.scale = glm::vec3(1.0f);
                                clObj.color = glm::vec4(1.0f);
                                clObj.meshId = clMeshId;
                                if (clMeshId < static_cast<int>(meshes.size())) {
                                    clObj.textureId = meshes[clMeshId].defaultTextureId;
                                    clObj.roughness = meshes[clMeshId].defaultRoughness;
                                    clObj.metallic = meshes[clMeshId].defaultMetallic;
                                }
                                sceneObjects.push_back(clObj);
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (genPreset == 5) // Floating Islands
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
            island.color = glm::vec4(1.0f);
            island.meshId = primitiveSphereMeshId;
            island.texturePath = "assets/grass.jpg";
            island.textureId = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
            island.usePBR = true;
            island.roughness = 0.85f;
            island.metallic = 0.05f;
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
                    std::string lowerMName = mName;
                    for (auto& c : lowerMName) c = static_cast<char>(tolower(c));
                    if (lowerMName.find("stone") != std::string::npos) {
                        setupStoneTransform(realObj, rx, ry + 0.75f, rz);
                    } else if (lowerMName.find("rock") != std::string::npos) {
                        setupRockTransform(realObj, rx, ry + 0.75f, rz);
                    } else if (lowerMName.find("bush") != std::string::npos && lowerMName.find("tree") == std::string::npos) {
                        setupBushTransform(realObj, rx, ry + 0.75f, rz);
                    } else if (lowerMName.find("tree_and_bush") != std::string::npos || lowerMName.find("trees_and_bush") != std::string::npos) {
                        realObj.position = glm::vec3(rx, ry + 0.75f, rz);
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        realObj.scale = glm::vec3(1.0f);
                    } else if (lowerMName.find("log") != std::string::npos) {
                        realObj.position = glm::vec3(rx, ry + 0.75f, rz);
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        realObj.scale = glm::vec3(1.0f);
                    } else {
                        realObj.position = glm::vec3(rx, ry + 1.2f, rz);
                        realObj.rotation = glm::vec3(0.0f, distRot(rng), 0.0f);
                        realObj.scale = glm::vec3(1.0f);
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
    else if (genPreset == 6) // 3D Maze
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
                            lantern.scale = glm::vec3(1.0f);
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
    else if (genPreset == 7) // Industrial Warehouse (warehouse.glb centerpiece)
    {
        // 1. Warehouse 3D Standard Architecture (assets/models/warehouse.glb)
        std::string warehousePath = "";
        if (std::filesystem::exists("assets/models/warehouse.glb")) warehousePath = "assets/models/warehouse.glb";
        else if (std::filesystem::exists("build/Release/assets/models/warehouse.glb")) warehousePath = "build/Release/assets/models/warehouse.glb";

        int warehouseMeshId = -1;
        if (!warehousePath.empty())
        {
            warehouseMeshId = assetManager->getOrLoadModelAsset(warehousePath);
        }

        std::string warehouseTexturePath = "";
        if (std::filesystem::exists("assets/models/warehouse.png")) warehouseTexturePath = "assets/models/warehouse.png";
        else if (std::filesystem::exists("assets/textures/warehouse.png")) warehouseTexturePath = "assets/textures/warehouse.png";
        else if (std::filesystem::exists("assets/textures/warehouse.jpg")) warehouseTexturePath = "assets/textures/warehouse.jpg";
        else if (std::filesystem::exists("build/Release/assets/models/warehouse.png")) warehouseTexturePath = "build/Release/assets/models/warehouse.png";

        int warehouseTexId = -1;
        if (!warehouseTexturePath.empty() && assetManager)
        {
            warehouseTexId = assetManager->getOrLoadTextureAsset(warehouseTexturePath);
        }

        if (warehouseMeshId >= 0)
        {
            // Main Warehouse Facility - 1:1 Metric Interior Logistics Depot
            // Model dimensions: Width 16m (X: -8 to +8), Height 5m (Y: 0 to 5), Length 46.2m (Z: -23.1 to +23.1)
            SceneObject mainWarehouse;
            mainWarehouse.id = static_cast<int>(sceneObjects.size());
            mainWarehouse.name = "🏭 Main_Warehouse_Facility";
            mainWarehouse.type = ObjectType::CUBE;
            mainWarehouse.position = glm::vec3(0.0f, 0.0f, 0.0f);
            mainWarehouse.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
            mainWarehouse.scale = glm::vec3(1.0f, 1.0f, 1.0f);
            mainWarehouse.color = glm::vec4(1.0f);
            mainWarehouse.meshId = warehouseMeshId;
            mainWarehouse.modelPath = warehousePath;
            mainWarehouse.texturePath = warehouseTexturePath;
            mainWarehouse.textureId = warehouseTexId;
            mainWarehouse.usePBR = true;
            mainWarehouse.roughness = 0.40f;
            mainWarehouse.metallic = 0.60f;
            if (mainWarehouse.textureId < 0 && warehouseMeshId < static_cast<int>(meshes.size()))
            {
                mainWarehouse.textureId = meshes[warehouseMeshId].defaultTextureId;
                mainWarehouse.roughness = meshes[warehouseMeshId].defaultRoughness;
                mainWarehouse.metallic = meshes[warehouseMeshId].defaultMetallic;
            }
            sceneObjects.push_back(mainWarehouse);
        }

        // 2. Real Warehouse Logistics Models (from assets/models/warehouse_item, 1:1 metric scale & PBR)
        auto placeWhItem = [&](const std::string& relPath, const std::string& name, const glm::vec3& pos, float rotY, const glm::vec3& scl = glm::vec3(1.0f)) {
            std::string actualPath = relPath;
            if (!std::filesystem::exists(actualPath)) {
                if (std::filesystem::exists("build/Release/" + relPath)) actualPath = "build/Release/" + relPath;
                else return;
            }
            int mId = assetManager->getOrLoadModelAsset(actualPath);
            if (mId < 0) return;

            SceneObject item;
            item.id = static_cast<int>(sceneObjects.size());
            item.name = name;
            item.type = ObjectType::CUBE;
            item.position = pos;
            item.rotation = glm::vec3(0.0f, rotY, 0.0f);
            item.scale = scl;
            item.color = glm::vec4(1.0f);
            item.meshId = mId;
            item.modelPath = actualPath;
            item.usePBR = true;
            if (mId < static_cast<int>(meshes.size())) {
                item.textureId = meshes[mId].defaultTextureId;
                item.roughness = meshes[mId].defaultRoughness;
                item.metallic = meshes[mId].defaultMetallic;
                if (item.textureId >= 0 && assetManager) {
                    item.texturePath = assetManager->getTexturePath(item.textureId);
                }
            }
            sceneObjects.push_back(item);
        };

        // A. High-Bay Industrial Shelving Aisles (Full pallet storage racks with goods)
        // Left Wing (West Racks)
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_Red_A/SM_Aisle_Red_A.glb", "🏗️ Shelving_Aisle_Red_L1", glm::vec3(-5.2f, 0.0f, 10.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_Red_A/SM_Aisle_Red_B.glb", "🏗️ Shelving_Aisle_Red_L2", glm::vec3(-5.2f, 0.0f, 6.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_White/SM_Aisle_White_A.glb", "🏗️ Shelving_Aisle_White_L3", glm::vec3(-5.2f, 0.0f, -4.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_White/SM_Aisle_White_B.glb", "🏗️ Shelving_Aisle_White_L4", glm::vec3(-5.2f, 0.0f, -8.5f), 90.0f);

        // Right Wing (East Racks)
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_White/SM_Aisle_White_A.glb", "🏗️ Shelving_Aisle_White_R1", glm::vec3(5.2f, 0.0f, 10.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_White/SM_Aisle_White_B.glb", "🏗️ Shelving_Aisle_White_R2", glm::vec3(5.2f, 0.0f, 6.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_Red_A/SM_Aisle_Red_A.glb", "🏗️ Shelving_Aisle_Red_R3", glm::vec3(5.2f, 0.0f, -4.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Aisles/SM_Aisle_Red_A/SM_Aisle_Red_B.glb", "🏗️ Shelving_Aisle_Red_R4", glm::vec3(5.2f, 0.0f, -8.5f), 90.0f);

        // B. Heavy Material Handling Equipment: Forklift & Pallet Trucks
        placeWhItem("assets/models/warehouse_item/SM_Industrial_Forklift_OBJ_v1_0/Obj/SM_Hyster_Forklift_Detailed.obj", "🚜 Industrial_Forklift_Detailed", glm::vec3(-2.2f, 0.0f, -14.5f), 25.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Pallet_Trucks_Hand_Trucks_Crates_Collection_GLB_v1_0/GLB/SM_IPalletTruck_Detailed/SM_IPalletTruck_Detailed/SM_HandTruck_Red.glb", "🛒 Pallet_Truck_Jack_Red_A", glm::vec3(2.0f, 0.0f, 9.8f), -25.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Pallet_Trucks_Hand_Trucks_Crates_Collection_GLB_v1_0/GLB/SM_IPalletTruck_Detailed/SM_IPalletTruck_Detailed/SM_HandTruck_Red.glb", "🛒 Pallet_Truck_Jack_Red_B", glm::vec3(-2.4f, 0.0f, -0.5f), 30.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Pallet_Trucks_Hand_Trucks_Crates_Collection_GLB_v1_0/GLB/SM_IHandTruck_Standard/SM_IHandTruck_Standard_Blue.glb", "🛒 TwoWheel_Hand_Truck_Blue", glm::vec3(2.4f, 0.0f, -1.2f), -75.0f);

        // C. Maintenance & Utility Station
        placeWhItem("assets/models/warehouse_item/Stylized_Industrial_Tool_Workbench_and_Tool_Set_GLB_v1_0/GLB/SM_Tool_Desk_Populated_D.glb", "🔧 Maintenance_Workbench_ToolStation", glm::vec3(6.8f, 0.0f, -14.0f), -90.0f);
        placeWhItem("assets/models/warehouse_item/Industrial_HVAC_Equipment_Collection_OBJ_v1_0/Obj/SM_Industrial_Air_Conditioner.obj", "❄️ Industrial_HVAC_Unit_West", glm::vec3(-6.8f, 0.0f, -18.5f), 90.0f);
        placeWhItem("assets/models/warehouse_item/Industrial_HVAC_Equipment_Collection_OBJ_v1_0/Obj/SM_Industrial_Air_Conditioner.obj", "❄️ Industrial_HVAC_Unit_East", glm::vec3(6.8f, 0.0f, -18.5f), -90.0f);

        // D. Pallet Logistics: Cement Bags & Carton Boxes
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Bags/SM_PalletteCementBags_A_D.glb", "📦 Pallet_Cement_Bags_Dock", glm::vec3(-2.0f, 0.0f, -18.5f), 10.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Bags/SM_PalletteCementBags_A_D.glb", "📦 Pallet_Cement_Bags_MidAisle", glm::vec3(-2.2f, 0.0f, 4.8f), 5.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Boxes/SM_PalletBoxes_A_D.glb", "📦 Pallet_Cargo_Boxes_Front", glm::vec3(2.0f, 0.0f, 8.5f), -25.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Boxes/SM_PalletBoxes_A_D.glb", "📦 Pallet_Cargo_Boxes_Mid", glm::vec3(2.2f, 0.0f, 4.8f), -10.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Boxes/SM_PalletBoxes_A_D.glb", "📦 Pallet_Cargo_Boxes_Dock", glm::vec3(1.8f, 0.0f, -18.5f), -5.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Boxes/SM_PalletBoxes_A_D.glb", "📦 Pallet_Cargo_Boxes_Corner", glm::vec3(6.8f, 0.0f, 14.5f), 5.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Pallets/SM_Pallet_A_D.glb", "🪵 Empty_Pallet_Base", glm::vec3(-6.8f, 0.0f, 14.5f), -10.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Warehouse_Shelving_Set_A_GLB_v1_0/GLB/SM_Stores/SM_Pallets/SM_Pallet_A_D.glb", "🪵 Empty_Pallet_StackTop", glm::vec3(-6.8f, 0.26f, 14.5f), -8.0f);

        // E. Hazardous Materials & Industrial Oil Barrels
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_Palleted_4_Barrels_01.obj", "🛢️ Palleted_Barrels_Dock", glm::vec3(2.4f, 0.0f, -13.5f), -20.0f);
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_Palleted_4_Barrels_01.obj", "🛢️ Palleted_Barrels_MidAisle", glm::vec3(-2.6f, 0.0f, 1.2f), 15.0f);
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_BarrelLabeled_01.obj", "🛢️ Hazard_Barrel_Labeled_A", glm::vec3(2.3f, 0.0f, 1.0f), -30.0f);
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_BarrelLabeled_01.obj", "🛢️ Hazard_Barrel_Labeled_B", glm::vec3(-2.8f, 0.0f, -11.5f), 80.0f);
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_BarrelLabeled_01.obj", "🛢️ Hazard_Barrel_Labeled_C", glm::vec3(-2.0f, 0.0f, 13.5f), 45.0f);

        // F. Industrial Wooden & Plastic Storage Crates
        placeWhItem("assets/models/warehouse_item/Stylized_Pallet_Trucks_Hand_Trucks_Crates_Collection_GLB_v1_0/GLB/SM_ICrate_Wood/SM_ICrate_Wood_Brown_V01.glb", "📦 Industrial_Wood_Crate_A", glm::vec3(1.8f, 0.0f, 2.2f), -15.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Pallet_Trucks_Hand_Trucks_Crates_Collection_GLB_v1_0/GLB/SM_ICrate_Wood/SM_ICrate_Wood_Brown_V01.glb", "📦 Industrial_Wood_Crate_B", glm::vec3(-1.7f, 0.0f, 12.0f), 12.0f);
        placeWhItem("assets/models/warehouse_item/Stylized_Pallet_Trucks_Hand_Trucks_Crates_Collection_GLB_v1_0/GLB/SM_IStackCrate/SM_IStackCrate_Red_V01.glb", "📦 Stackable_Plastic_Crate", glm::vec3(-1.7f, 0.47f, 12.0f), 12.0f);

        // G. Additional Specialized OBJ Industrial Props (Pallet Wood & Chemical Barrels)
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_Pallet_Wood_01.obj", "🪵 Wooden_Pallet_Staging_OBJ", glm::vec3(2.4f, 0.0f, -11.5f), 15.0f);
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_PlasticBarrel_L1_C1.obj", "🛢️ Plastic_Chemical_Barrel_OBJ", glm::vec3(-2.5f, 0.0f, 8.5f), -35.0f);
        placeWhItem("assets/models/warehouse_item/SM_Barrels_Palleted_OBJ_v1_0/Obj/SM_Industrial_Barrel_01_Damaged.obj", "🛢️ Damaged_Steel_Barrel_OBJ", glm::vec3(2.5f, 0.0f, -8.5f), 65.0f);

        // 5. Tactical CQB Targets & IPSC Stands (ShootingTarget.fbx, IPSC_Metric_Target.obj, Pepper_Popper.obj)
        // 5. Tactical CQB Targets & IPSC Stands (ShootingTarget.obj, IPSC_Metric_Target.obj, Pepper_Popper.obj)
        std::string shootTargetObj = "";
        if (std::filesystem::exists("assets/models/ShootingTarget.obj")) shootTargetObj = "assets/models/ShootingTarget.obj";
        else if (std::filesystem::exists("build/Release/assets/models/ShootingTarget.obj")) shootTargetObj = "build/Release/assets/models/ShootingTarget.obj";
        else if (std::filesystem::exists("assets/models/ShootingTarget.fbx")) shootTargetObj = "assets/models/ShootingTarget.fbx";

        int shootTargetMeshId = -1;
        if (!shootTargetObj.empty()) shootTargetMeshId = assetManager->getOrLoadModelAsset(shootTargetObj);

        struct TargetDef { glm::vec3 pos; float rotY; const char* name; };
        std::vector<TargetDef> shootTargets = {
            { glm::vec3(0.0f, 0.0f, -17.0f), 0.0f, "🎯 Warehouse_CQB_Target_FarEnd" },
            { glm::vec3(-2.8f, 0.0f, -10.0f), 15.0f, "🎯 Warehouse_CQB_Target_FlankLeft" },
            { glm::vec3(2.8f, 0.0f, -4.0f), -15.0f, "🎯 Warehouse_CQB_Target_FlankRight" },
            { glm::vec3(-1.8f, 0.0f, 2.0f), 5.0f, "🎯 Warehouse_CQB_Target_MidLeft" },
            { glm::vec3(2.0f, 0.0f, 8.0f), -5.0f, "🎯 Warehouse_CQB_Target_CloseRight" }
        };

        for (const auto& st : shootTargets)
        {
            SceneObject tObj;
            tObj.id = static_cast<int>(sceneObjects.size());
            tObj.name = st.name;
            tObj.type = ObjectType::CUBE;
            tObj.position = st.pos;
            tObj.rotation = glm::vec3(0.0f, st.rotY, 0.0f);
            tObj.scale = glm::vec3(1.0f);
            tObj.color = glm::vec4(1.0f);
            tObj.usePBR = true;
            if (shootTargetMeshId >= 0) {
                tObj.meshId = shootTargetMeshId;
                tObj.modelPath = shootTargetObj;
                if (shootTargetMeshId < static_cast<int>(meshes.size())) {
                    tObj.textureId = meshes[shootTargetMeshId].defaultTextureId;
                    tObj.roughness = meshes[shootTargetMeshId].defaultRoughness;
                    tObj.metallic = meshes[shootTargetMeshId].defaultMetallic;
                    if (tObj.textureId >= 0 && assetManager) {
                        tObj.texturePath = assetManager->getTexturePath(tObj.textureId);
                    }
                }
                if (tObj.textureId < 0) {
                    tObj.texturePath = "assets/models/ShootingTarget.png";
                    tObj.textureId = assetManager->getOrLoadTextureAsset("assets/models/ShootingTarget.png");
                }
            } else {
                tObj.meshId = primitiveCubeMeshId;
                tObj.scale = glm::vec3(0.8f, 1.75f, 0.1f);
                tObj.position.y = 0.875f;
            }
            sceneObjects.push_back(tObj);
        }

        // Additional IPSC Metric Targets
        std::string ipscPath = "";
        if (std::filesystem::exists("assets/models/IPSC_Metric_Target.obj")) ipscPath = "assets/models/IPSC_Metric_Target.obj";
        else if (std::filesystem::exists("build/Release/assets/models/IPSC_Metric_Target.obj")) ipscPath = "build/Release/assets/models/IPSC_Metric_Target.obj";

        int ipscMeshId = -1;
        if (!ipscPath.empty()) ipscMeshId = assetManager->getOrLoadModelAsset(ipscPath);

        if (ipscMeshId >= 0)
        {
            std::vector<TargetDef> ipscTargets = {
                { glm::vec3(-1.2f, 0.0f, -14.0f), 10.0f, "🎯 IPSC_Warehouse_Target_CoverA" },
                { glm::vec3(1.5f, 0.0f, -7.0f), -15.0f, "🎯 IPSC_Warehouse_Target_CoverB" },
                { glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, "🎯 IPSC_Warehouse_Target_Center" }
            };
            for (const auto& ip : ipscTargets)
            {
                SceneObject ipObj;
                ipObj.id = static_cast<int>(sceneObjects.size());
                ipObj.name = ip.name;
                ipObj.type = ObjectType::CUBE;
                ipObj.position = ip.pos;
                ipObj.rotation = glm::vec3(0.0f, ip.rotY, 0.0f);
                ipObj.scale = glm::vec3(1.0f);
                ipObj.color = glm::vec4(1.0f);
                ipObj.meshId = ipscMeshId;
                ipObj.modelPath = ipscPath;
                ipObj.usePBR = true;
                if (ipscMeshId < static_cast<int>(meshes.size())) {
                    ipObj.textureId = meshes[ipscMeshId].defaultTextureId;
                    ipObj.roughness = meshes[ipscMeshId].defaultRoughness;
                    ipObj.metallic = meshes[ipscMeshId].defaultMetallic;
                    if (ipObj.textureId >= 0 && assetManager) {
                        ipObj.texturePath = assetManager->getTexturePath(ipObj.textureId);
                    }
                }
                sceneObjects.push_back(ipObj);
            }
        }

        // Steel Pepper Poppers
        std::string popperPath = "";
        if (std::filesystem::exists("assets/models/Pepper_Popper.obj")) popperPath = "assets/models/Pepper_Popper.obj";
        else if (std::filesystem::exists("build/Release/assets/models/Pepper_Popper.obj")) popperPath = "build/Release/assets/models/Pepper_Popper.obj";

        int popperMeshId = -1;
        if (!popperPath.empty()) popperMeshId = assetManager->getOrLoadModelAsset(popperPath);

        if (popperMeshId >= 0)
        {
            std::vector<TargetDef> poppers = {
                { glm::vec3(-1.8f, 0.0f, 4.0f), 0.0f, "🔘 Steel_Pepper_Popper_1" },
                { glm::vec3(1.8f, 0.0f, 4.0f), 0.0f, "🔘 Steel_Pepper_Popper_2" }
            };
            for (const auto& pop : poppers)
            {
                SceneObject popObj;
                popObj.id = static_cast<int>(sceneObjects.size());
                popObj.name = pop.name;
                popObj.type = ObjectType::CUBE;
                popObj.position = pop.pos;
                popObj.rotation = glm::vec3(0.0f, pop.rotY, 0.0f);
                popObj.scale = glm::vec3(1.0f);
                popObj.color = glm::vec4(0.92f, 0.95f, 0.98f, 1.0f);
                popObj.meshId = popperMeshId;
                popObj.modelPath = popperPath;
                popObj.usePBR = true;
                popObj.roughness = 0.4f;
                popObj.metallic = 0.8f;
                if (popperMeshId < static_cast<int>(meshes.size())) {
                    popObj.textureId = meshes[popperMeshId].defaultTextureId;
                    if (popObj.textureId >= 0 && assetManager) {
                        popObj.texturePath = assetManager->getTexturePath(popObj.textureId);
                    }
                }
                sceneObjects.push_back(popObj);
            }
        }

        // 6. Interior Fluorescent Ceiling Lighting (Lights hanging along arched roof, casting reflections onto wet floor)
        if (genIncludeLights)
        {
            struct FloodlightDef { glm::vec3 pos; glm::vec3 color; float intensity; const char* name; };
            std::vector<FloodlightDef> lights = {
                { glm::vec3(0.0f, 4.3f, 14.0f), glm::vec3(0.92f, 0.96f, 1.0f), 2.8f, "💡 Ceiling_Fluorescent_Front" },
                { glm::vec3(0.0f, 4.3f, 4.0f), glm::vec3(0.92f, 0.96f, 1.0f), 3.0f, "💡 Ceiling_Fluorescent_MidFront" },
                { glm::vec3(0.0f, 4.3f, -5.0f), glm::vec3(0.92f, 0.96f, 1.0f), 3.0f, "💡 Ceiling_Fluorescent_MidBack" },
                { glm::vec3(0.0f, 4.3f, -14.0f), glm::vec3(0.92f, 0.96f, 1.0f), 2.8f, "💡 Ceiling_Fluorescent_FarBack" }
            };

            for (const auto& lt : lights)
            {
                SceneObject lObj;
                lObj.id = static_cast<int>(sceneObjects.size());
                lObj.name = lt.name;
                lObj.type = ObjectType::LIGHT;
                lObj.position = lt.pos;
                lObj.scale = glm::vec3(0.35f);
                lObj.color = glm::vec4(lt.color, 1.0f);
                lObj.meshId = primitiveCubeMeshId;
                auto lComp = std::make_shared<LightComponent>();
                lComp->color = lObj.color;
                lComp->intensity = lt.intensity;
                lObj.components.push_back(lComp);
                sceneObjects.push_back(lObj);
            }
        }
    }
    else if (genPreset == 8) // Đường Phố Đô Thị (NYC Urban Street - 1:1 Scale & Realistic Textures)
    {
        auto placeStreetModel = [&](const std::string& relPath, const std::string& name, const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.0f), const glm::vec3& scl = glm::vec3(1.0f), float roughness = 0.5f, float metallic = 0.0f) {
            std::string actualPath = relPath;
            if (!std::filesystem::exists(actualPath)) {
                if (std::filesystem::exists("build/Release/" + relPath)) actualPath = "build/Release/" + relPath;
                else if (std::filesystem::exists("assets/models/" + relPath)) actualPath = "assets/models/" + relPath;
                else return;
            }
            int mId = assetManager->getOrLoadModelAsset(actualPath);
            if (mId < 0) return;

            SceneObject item;
            item.id = static_cast<int>(sceneObjects.size());
            item.name = name;
            item.type = ObjectType::CUBE;
            item.position = pos;
            item.rotation = rot;
            item.scale = scl;
            item.color = glm::vec4(1.0f);
            item.meshId = mId;
            item.modelPath = relPath;
            item.usePBR = true;
            item.roughness = roughness;
            item.metallic = metallic;

            const Mesh* meshPtr = assetManager->getMesh(mId);
            if (meshPtr) {
                item.textureId = meshPtr->defaultTextureId;
                if (meshPtr->defaultRoughness > 0.0f || meshPtr->defaultMetallic > 0.0f) {
                    item.roughness = meshPtr->defaultRoughness;
                    item.metallic = meshPtr->defaultMetallic;
                }
                if (item.textureId >= 0 && assetManager) {
                    item.texturePath = assetManager->getTexturePath(item.textureId);
                }
            }
            sceneObjects.push_back(item);
        };

        struct StreetProp {
            const char* model;
            const char* name;
            glm::vec3 pos;
            glm::vec3 rot;
            glm::vec3 scl;
            float roughness;
            float metallic;
            int category; // 0: core, 1: tree, 2: building
        };

        static const StreetProp streetProps[] = {
            // Core Street Infrastructure (1:1 Metric Scale)
            { "assets/models/nyc_street/street_road_asphalt.glb", "Street_Road_Asphalt", glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.85f, 0.05f, 0 },
            { "assets/models/nyc_street/sidewalk_curb.glb", "Sidewalk_Left_Curb", glm::vec3(-6.5f, 0.09f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.75f, 0.05f, 0 },
            { "assets/models/nyc_street/sidewalk_curb.glb", "Sidewalk_Right_Curb", glm::vec3(6.5f, 0.09f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.75f, 0.05f, 0 },
            { "assets/models/nyc_street/waterfront_pier.glb", "Waterfront_Pier_Promenade", glm::vec3(0.0f, 0.09f, -26.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.75f, 0.05f, 0 },
            { "assets/models/nyc_street/waterfront_railing.glb", "Waterfront_Pier_Railing", glm::vec3(0.0f, 0.0f, -27.8f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.40f, 0.80f, 0 },

            // Vehicles (Calibrated Shooting Lane Clearances) - Grounded at Y=0.0f on road
            { "assets/models/nyc_street/nyc_taxi.glb", "NYC_Taxi_4W92", glm::vec3(-2.2f, 0.0f, 6.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.25f, 0.10f, 0 },
            { "assets/models/nyc_street/nypd_cruiser.glb", "NYPD_Cruiser_1_Echo", glm::vec3(-1.6f, 0.0f, -3.6f), glm::vec3(0.0f, 20.05f, 0.0f), glm::vec3(1.0f), 0.25f, 0.10f, 0 },
            { "assets/models/nyc_street/nypd_cruiser.glb", "NYPD_Cruiser_2_Bravo", glm::vec3(2.0f, 0.0f, -7.8f), glm::vec3(0.0f, -17.19f, 0.0f), glm::vec3(1.0f), 0.25f, 0.10f, 0 },

            // Police Barricades & Cordon Line
            { "assets/models/nyc_street/nypd_barricade.glb", "NYPD_Barricade_1", glm::vec3(0.8f, 0.0f, 0.4f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.65f, 0.05f, 0 },
            { "assets/models/nyc_street/nypd_barricade.glb", "NYPD_Barricade_2", glm::vec3(3.0f, 0.0f, 0.4f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.65f, 0.05f, 0 },
            { "assets/models/nyc_street/police_cordon_tape.glb", "NYPD_Police_Cordon_Tape", glm::vec3(1.04f, 0.0f, 1.19f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.35f, 0.05f, 0 },

            // High-Visibility Traffic Cones
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_1", glm::vec3(-3.9f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_2", glm::vec3(-2.6f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_3", glm::vec3(-1.3f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_4", glm::vec3(0.0f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_5", glm::vec3(1.3f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_6", glm::vec3(2.6f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },
            { "assets/models/nyc_street/police_traffic_cone.glb", "Police_TrafficCone_7", glm::vec3(3.9f, 0.38f, 1.5f), glm::vec3(0.0f), glm::vec3(1.0f), 0.45f, 0.05f, 0 },

            // Sidewalk Furniture & Street Props
            { "assets/models/nyc_street/nyc_fire_hydrant.glb", "NYC_FireHydrant", glm::vec3(-4.85f, 0.18f, 6.5f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(1.0f), 0.45f, 0.60f, 0 },
            { "assets/models/nyc_street/nyc_bus_stop_shelter.glb", "NYC_BusStop_Shelter", glm::vec3(5.5f, 0.18f, -2.5f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.35f, 0.40f, 0 },
            { "assets/models/nyc_street/nyc_newsbox_a.glb", "NYC_NewsBox_VarA", glm::vec3(-4.88f, 0.18f, 4.9f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(1.0f), 0.45f, 0.50f, 0 },
            { "assets/models/nyc_street/nyc_newsbox_b.glb", "NYC_NewsBox_VarB", glm::vec3(-4.88f, 0.18f, 4.2f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(1.0f), 0.45f, 0.50f, 0 },
            { "assets/models/nyc_street/nyc_street_sign.glb", "NYC_StreetSign_Left", glm::vec3(-4.85f, 0.18f, 1.8f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(1.0f), 0.45f, 0.50f, 0 },
            { "assets/models/nyc_street/nyc_street_sign.glb", "NYC_StreetSign_Right", glm::vec3(5.39f, 0.18f, 1.8f), glm::vec3(0.0f, -45.0f, 0.0f), glm::vec3(1.0f), 0.45f, 0.50f, 0 },
            { "assets/models/nyc_street/nyc_manhole_cover.glb", "NYC_Manhole_Cover", glm::vec3(-1.8f, 0.0f, 7.2f), glm::vec3(0.0f, 15.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.80f, 0 },
            { "assets/models/nyc_street/nyc_sewer_grate.glb", "NYC_SewerGrate_Left", glm::vec3(-4.42f, 0.0f, 5.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.80f, 0 },

            // Street Trees (category 1)
            { "assets/models/nyc_street/nyc_street_tree.glb", "NYC_StreetTree_1", glm::vec3(-6.2f, 0.18f, 9.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.02f, 1 },
            { "assets/models/nyc_street/nyc_street_tree.glb", "NYC_StreetTree_2", glm::vec3(-6.2f, 0.18f, -2.5f), glm::vec3(0.0f, 120.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.02f, 1 },
            { "assets/models/nyc_street/nyc_street_tree.glb", "NYC_StreetTree_3", glm::vec3(-6.2f, 0.18f, -14.5f), glm::vec3(0.0f, 240.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.02f, 1 },
            { "assets/models/nyc_street/nyc_street_tree.glb", "NYC_StreetTree_4", glm::vec3(6.2f, 0.18f, -8.5f), glm::vec3(0.0f, 45.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.02f, 1 },
            { "assets/models/nyc_street/nyc_street_tree.glb", "NYC_StreetTree_5", glm::vec3(6.2f, 0.18f, -15.5f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.70f, 0.02f, 1 },

            // Street Lampposts
            { "assets/models/nyc_street/nyc_lamppost.glb", "NYC_Lamppost_Left_1", glm::vec3(-4.0f, 0.18f, 11.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.35f, 0.70f, 0 },
            { "assets/models/nyc_street/nyc_lamppost.glb", "NYC_Lamppost_Left_2", glm::vec3(-4.0f, 0.18f, -7.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), 0.35f, 0.70f, 0 },
            { "assets/models/nyc_street/nyc_lamppost.glb", "NYC_Lamppost_Right_1", glm::vec3(4.0f, 0.18f, 7.5f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.35f, 0.70f, 0 },
            { "assets/models/nyc_street/nyc_lamppost.glb", "NYC_Lamppost_Right_2", glm::vec3(4.85f, 0.18f, -12.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.35f, 0.70f, 0 },

            // Merged Street Buildings (category 2)
            { "assets/models/nyc_street/building_left_near.glb", "Building_Left_Front", glm::vec3(-10.0f, 0.18f, 12.0f), glm::vec3(0.0f), glm::vec3(1.0f), 0.5f, 0.0f, 2 },
            { "assets/models/nyc_street/building_left_far.glb", "Building_Left_Back", glm::vec3(-10.0f, 0.18f, -12.0f), glm::vec3(0.0f), glm::vec3(1.0f), 0.5f, 0.0f, 2 },
            { "assets/models/nyc_street/building_right_near.glb", "Building_Right_Front", glm::vec3(10.0f, 0.18f, 12.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.5f, 0.0f, 2 },
            { "assets/models/nyc_street/building_right_near.glb", "Building_Right_Mid", glm::vec3(10.0f, 0.18f, -12.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.5f, 0.0f, 2 },
            { "assets/models/nyc_street/building_right_far.glb", "Building_Right_Far", glm::vec3(10.0f, 0.18f, -36.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.5f, 0.0f, 2 },

            // Depth Buildings (from 03_Street_Buildings in Blender)
            { "assets/models/nyc_street/building_depth_midrise.glb", "Bld_Right_Depth_MidRise", glm::vec3(45.0f, 0.18f, -26.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.2f), 0.5f, 0.0f, 2 },
            { "assets/models/nyc_street/building_depth_small.glb", "Bld_Right_Depth_Small", glm::vec3(45.0f, 0.18f, -44.0f), glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(1.0f), 0.5f, 0.0f, 2 },
            { "assets/models/nyc_street/building_depth_tower.glb", "Bld_Right_Depth_Tower", glm::vec3(45.0f, 0.18f, -8.0f), glm::vec3(0.0f, 90.0f, 0.0f), glm::vec3(0.7f), 0.5f, 0.0f, 2 },

            // Background Manhattan Skyline Panorama
            { "assets/models/nyc_street/background_manhattan.glb", "NewYork_Manhattan_Skyline", glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f), 0.8f, 0.1f, 2 }
        };

        for (const auto& sp : streetProps)
        {
            if (sp.category == 1 && !genIncludeTrees) continue;
            if (sp.category == 2 && !genIncludeBuildings) continue;
            placeStreetModel(sp.model, sp.name, sp.pos, sp.rot, sp.scl, sp.roughness, sp.metallic);
        }

        // Night / Evening Point Lights (Streetlights and Police Emergency Strobes)
        if (genIncludeLights)
        {
            struct StreetLightDef {
                const char* name;
                glm::vec3 pos;
                glm::vec4 color;
                float intensity;
            };
            static const StreetLightDef lights[] = {
                { "💡 Lamppost_Light_1", glm::vec3(-4.0f, 4.18f, 11.0f), glm::vec4(1.0f, 0.90f, 0.75f, 1.0f), 2.5f },
                { "💡 Lamppost_Light_2", glm::vec3(-4.0f, 4.18f, -7.0f), glm::vec4(1.0f, 0.90f, 0.75f, 1.0f), 2.5f },
                { "💡 Lamppost_Light_3", glm::vec3(4.0f, 4.18f, 7.5f),   glm::vec4(1.0f, 0.90f, 0.75f, 1.0f), 2.5f },
                { "💡 Lamppost_Light_4", glm::vec3(4.0f, 4.18f, -12.0f), glm::vec4(1.0f, 0.90f, 0.75f, 1.0f), 2.5f },
                { "💡 Police_1_Red",     glm::vec3(-1.85f, 1.95f, -3.2f), glm::vec4(1.0f, 0.05f, 0.05f, 1.0f), 3.5f },
                { "💡 Police_1_Blue",    glm::vec3(-1.35f, 1.95f, -3.2f), glm::vec4(0.05f, 0.25f, 1.0f, 1.0f), 3.5f },
                { "💡 Police_2_Red",     glm::vec3(1.75f, 1.95f, -7.4f),  glm::vec4(1.0f, 0.05f, 0.05f, 1.0f), 3.5f },
                { "💡 Police_2_Blue",    glm::vec3(2.25f, 1.95f, -7.4f),  glm::vec4(0.05f, 0.25f, 1.0f, 1.0f), 3.5f }
            };

            for (const auto& lt : lights)
            {
                SceneObject lObj;
                lObj.id = static_cast<int>(sceneObjects.size());
                lObj.name = lt.name;
                lObj.type = ObjectType::LIGHT;
                lObj.position = lt.pos;
                lObj.scale = glm::vec3(0.2f);
                lObj.color = lt.color;
                lObj.meshId = primitiveCubeMeshId;
                auto lComp = std::make_shared<LightComponent>();
                lComp->color = lObj.color;
                lComp->intensity = lt.intensity;
                lObj.components.push_back(lComp);
                sceneObjects.push_back(lObj);
            }
        }
    }

    glm::vec3 mainCamP(0.0f, 3.2f, 14.5f);
    glm::vec3 mainCamT(0.0f, 1.2f, 0.0f);
    glm::vec3 sceneCamT(0.0f, 1.0f, 0.0f);
    float sceneCamDist = 14.0f;
    float sceneRotX = 20.0f;
    float sceneRotY = 40.0f;

    if (genPreset == 7)
    {
        // Interior view: standing inside warehouse looking down the 46m hall toward the rollup door
        mainCamP = glm::vec3(0.0f, 1.75f, 18.0f);
        mainCamT = glm::vec3(0.0f, 1.85f, -15.0f);
        sceneCamT = glm::vec3(0.0f, 2.0f, 2.0f);
        sceneCamDist = 18.0f;
        sceneRotX = 8.0f;
        sceneRotY = 0.0f;
        scene->setPlayerStartPos(glm::vec3(0.0f, 0.0f, 17.0f));
    }
    else if (genPreset == 8)
    {
        // NYC Urban Street View: Calibrated eye-level shooting lane looking down the street toward the bay
        mainCamP = glm::vec3(-0.5f, 1.62f, 15.0f);
        mainCamT = glm::vec3(-0.5f, 1.40f, -5.0f);
        sceneCamT = glm::vec3(-0.5f, 1.50f, -5.0f);
        sceneCamDist = 20.0f;
        sceneRotX = 16.0f;
        sceneRotY = 0.0f;
        scene->setPlayerStartPos(glm::vec3(-0.5f, 0.0f, 14.5f));
    }

    float mainCamFarDist = (genPreset == 8) ? 500.0f : 120.0f;
    float mainCamFov = (genPreset == 8) ? 60.0f : 45.0f;
    if (gizmoSystem)
    {
        // Preserve and set user's calibrated camera settings for the 2.4m x 2.0m screen
        gizmoSystem->mainCameraPos = mainCamP;
        gizmoSystem->mainCameraTarget = mainCamT;
        gizmoSystem->mainCameraFov = mainCamFov;
        gizmoSystem->mainCameraNear = 0.1f;
        gizmoSystem->mainCameraFar = mainCamFarDist;
        gizmoSystem->gameAspectRatioMode = GameViewAspectRatioMode::PHOTO_2M4_X_2M;

        gizmoSystem->sceneCameraTarget = sceneCamT;
        gizmoSystem->sceneRotationX = sceneRotX;
        gizmoSystem->sceneRotationY = sceneRotY;
        gizmoSystem->sceneCameraDistance = sceneCamDist;
    }

    auto& cam = scene->getCameraSettings();
    cam.mainCameraPos = mainCamP;
    cam.mainCameraTarget = mainCamT;
    cam.mainCameraFov = mainCamFov;
    cam.mainCameraNear = 0.1f;
    cam.mainCameraFar = mainCamFarDist;
    cam.aspectRatioMode = 0; // PHOTO_2M4_X_2M
    cam.sceneCameraTarget = sceneCamT;
    cam.sceneRotationX = sceneRotX;
    cam.sceneRotationY = sceneRotY;
    cam.sceneCameraDistance = sceneCamDist;
    cam.hasSavedCamera = true;

    std::string envName = "Tactical Outpost";
    if (genPreset == 0) envName = "Tactical Outpost";
    else if (genPreset == 1) envName = "Forest Clearing";
    else if (genPreset == 2) envName = "Desert Dunes";
    else if (genPreset == 3) envName = "Cyberpunk City";
    else if (genPreset == 4) envName = "Medieval Citadel";
    else if (genPreset == 5) envName = "Floating Islands";
    else if (genPreset == 6) envName = "3D Maze";
    else if (genPreset == 7) envName = "Industrial Warehouse";
    else if (genPreset == 8) envName = "NYC Street Scene";

    scene->setCurrentEnvironmentName(envName);
    std::string envFileName = Scene::getEnvironmentFileName(envName);
    scene->setCurrentScenePath(envFileName);

    scene->saveScene(envFileName, assetManager, gizmoSystem);
    if (genPreset == 8)
    {
        scene->saveScene("scene_nyc_street.json", assetManager, gizmoSystem);
    }
    scene->exportToJMonkeyEngine("scene_jme.json", assetManager, gizmoSystem);

    printf("[3D Scene Generator] Created & saved scene '%s' (%s) with %zu entities!\n", envName.c_str(), envFileName.c_str(), sceneObjects.size());
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
            "🎯 1. Chiến Hào & Trường Bắn Sa Mạc (Tactical Outpost - scene_demo)",
            "🌲 2. Cánh Đồng Trống Trong Rừng (Forest Clearing - Sống Cảnh 2)",
            "🏜️ 3. Sa Mạc Cổ Tích & Kim Tự Tháp (Desert Dunes & Ruins)",
            "🏙️ 4. Thành Phố Tương Lai (Cyberpunk City)",
            "🏰 5. Ngôi Làng & Lâu Đài (Medieval Citadel)",
            "🌌 6. Quần Đảo Phao (Floating Archipelago)",
            "🌀 7. Mê Cung 3D (3D Dungeon Maze)",
            "🏭 8. Kho Hàng Thực Tế (Industrial Warehouse - warehouse.glb)",
            "🏙️ 9. Đường Phố Đô Thị (NYC Urban Street - 1:1 Scale & Realistic Textures)"
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
            generate3DScene(scene, assetManager, gizmoSystem);
        }
        ImGui::PopStyleColor(2);

        if (ImGui::Button(" 📷 Căn Cam Chiếu Sinh Cảnh (Khung 2.4m × 2.0m) ", ImVec2(-1, 28)))
        {
            if (gizmoSystem)
            {
                gizmoSystem->frameScenicPhotoView(*scene, 0);
            }
        }

        std::string currentPresetEnv = (genPreset == 0) ? "Tactical Outpost" :
                                       (genPreset == 1) ? "Forest Clearing" :
                                       (genPreset == 2) ? "Desert Dunes" :
                                       (genPreset == 3) ? "Cyberpunk City" :
                                       (genPreset == 4) ? "Medieval Citadel" :
                                       (genPreset == 5) ? "Floating Islands" :
                                       (genPreset == 6) ? "3D Maze" :
                                       (genPreset == 7) ? "Industrial Warehouse" : "NYC Street Scene";
        std::string currentEnvFile = Scene::getEnvironmentFileName(currentPresetEnv);
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "💾 File Môi Trường: %s", currentEnvFile.c_str());

        if (ImGui::Button((" 💾 Lưu Scene Môi Trường (" + currentEnvFile + ") ").c_str(), ImVec2(-1, 28)))
        {
            scene->setCurrentEnvironmentName(currentPresetEnv);
            scene->setCurrentScenePath(currentEnvFile);
            scene->saveScene(currentEnvFile, assetManager, gizmoSystem);
            if (genPreset == 8)
            {
                scene->saveScene("scene_nyc_street.json", assetManager, gizmoSystem);
            }
            scene->exportToJMonkeyEngine("scene_jme.json", assetManager, gizmoSystem);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.2f, 1.0f), "🐵 Xuất Cảnh Cho jMonkeyEngine (Cách 1):");
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.85f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.65f, 0.95f, 1.0f));
        if (ImGui::Button(" 📦 XUẤT CHO jMonkeyEngine (scene_jme.json) ", ImVec2(-1, 32)))
        {
            if (scene->exportToJMonkeyEngine("scene_jme.json", assetManager, gizmoSystem))
            {
                jmeExportNotifyTimer = 5.0f;
                jmeExportStatusMsg = "✅ Đã xuất scene_jme.json, terrain.obj và SceneLoader.java!";
            }
        }
        ImGui::PopStyleColor(2);

        if (jmeExportNotifyTimer > 0.0f)
        {
            jmeExportNotifyTimer -= ImGui::GetIO().DeltaTime;
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", jmeExportStatusMsg.c_str());
        }

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
                    else if (lowerName.find("tree_and_bush") != std::string::npos || lowerName.find("trees_and_bush") != std::string::npos || lowerName.find("grove") != std::string::npos) icon = "🌳 ";
                    else if (lowerName.find("bush") != std::string::npos) icon = "🌿 ";
                    else if (lowerName.find("stone") != std::string::npos) icon = "🪨 ";
                    else if (lowerName.find("rock") != std::string::npos) icon = "🪨 ";
                    else if (lowerName.find("tree") != std::string::npos) icon = "🌲 ";
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
                    std::string stemName = p.stem().string();
                    std::string lowerStem = stemName;
                    for (auto& c : lowerStem) c = static_cast<char>(tolower(c));

                    std::string mIcon = "🧊 ";
                    if (lowerStem.find("tree_and_bush") != std::string::npos || lowerStem.find("trees_and_bush") != std::string::npos) mIcon = "🌳 ";
                    else if (lowerStem.find("bush") != std::string::npos) mIcon = "🌿 ";
                    else if (lowerStem.find("stone") != std::string::npos) mIcon = "🪨 ";
                    else if (lowerStem.find("rock") != std::string::npos) mIcon = "🪨 ";
                    else if (lowerStem.find("tree") != std::string::npos) mIcon = "🌲 ";

                    std::string mLabel = mIcon + p.filename().string();
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

                            newObj.scale = glm::vec3(1.0f);
                            newObj.rotation = glm::vec3(0.0f);
                            newObj.position = glm::vec3(0.0f);
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
