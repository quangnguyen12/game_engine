#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <cmath>
#include "scene/Scene.h"
#include "physics/PhysicsSystem.h"
#include "assets/AssetManager.h"
#include "editor/GizmoSystem.h"
#include "renderer/VulkanRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include "stb_image_write.h"

Scene::Scene()
    : selectedObjectIndex(0),
      playerStartPos(0.0f, 0.0f, 0.0f)
{
}

SceneObject* Scene::getSelectedObject()
{
    if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        return &sceneObjects[selectedObjectIndex];
    }
    return nullptr;
}

const SceneObject* Scene::getSelectedObject() const
{
    if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        return &sceneObjects[selectedObjectIndex];
    }
    return nullptr;
}

glm::mat4 Scene::getWorldMatrix(int index) const
{
    return calculateWorldMatrix(sceneObjects, index);
}

glm::mat4 Scene::calculateWorldMatrix(const std::vector<SceneObject>& objects, int index)
{
    if (index < 0 || index >= static_cast<int>(objects.size())) return glm::mat4(1.0f);

    const auto& obj = objects[index];
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.position);
    model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, obj.scale);

    if (obj.parentId >= 0 && obj.parentId < static_cast<int>(objects.size()) && obj.parentId != index)
    {
        return calculateWorldMatrix(objects, obj.parentId) * model;
    }

    return model;
}

void Scene::saveHistory()
{
    undoStack.push_back(sceneObjects);
    redoStack.clear();
}

void Scene::undo()
{
    if (!undoStack.empty())
    {
        redoStack.push_back(sceneObjects);
        sceneObjects = undoStack.back();
        undoStack.pop_back();
        if (selectedObjectIndex >= static_cast<int>(sceneObjects.size()))
        {
            selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
        }
    }
}

void Scene::redo()
{
    if (!redoStack.empty())
    {
        undoStack.push_back(sceneObjects);
        sceneObjects = redoStack.back();
        redoStack.pop_back();
        if (selectedObjectIndex >= static_cast<int>(sceneObjects.size()))
        {
            selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
        }
    }
}

void Scene::savePlayModeState()
{
    for (auto& obj : sceneObjects)
    {
        obj.savedPosition = obj.position;
        obj.savedRotation = obj.rotation;
        obj.savedScale = obj.scale;
    }
}

void Scene::restoreEditModeState(PhysEngine* physEngine)
{
    for (auto& obj : sceneObjects)
    {
        obj.position = obj.savedPosition;
        obj.rotation = obj.savedRotation;
        obj.scale = obj.savedScale;
        obj.velocity = glm::vec3(0.0f);
        if (obj.bodyData && physEngine)
        {
            physEngine->setBodyPosition(obj.bodyData, obj.position);
            glm::quat rotQuat = glm::quat(glm::radians(obj.rotation));
            physEngine->setBodyRotation(obj.bodyData, rotQuat);
            physEngine->setLinearVelocity(obj.bodyData, glm::vec3(0.0f));
            physEngine->setAngularVelocity(obj.bodyData, glm::vec3(0.0f));
        }
    }
}

static std::string findMatchingModelAsset(const std::string& name)
{
    std::string lowerName = name;
    for (auto& c : lowerName) c = static_cast<char>(tolower(c));

    // Specific known mappings for procedural & downloaded assets
    if (lowerName.find("tree_pine") != std::string::npos) return "assets/models/tree_pine.obj";
    if (lowerName.find("tree_cedar") != std::string::npos) return "assets/models/tree_cedar.obj";
    if (lowerName.find("tree_redwood") != std::string::npos) return "assets/models/tree_redwood.obj";
    if (lowerName.find("tree_oak") != std::string::npos) return "assets/models/tree_oak.obj";
    if (lowerName.find("tree_mossy") != std::string::npos) return "assets/models/tree_mossy.obj";
    if (lowerName.find("tree_savanna") != std::string::npos) return "assets/models/tree_savanna.obj";
    if (lowerName.find("tree9") != std::string::npos) return "assets/models/tree9.obj";
    if (lowerName.find("stone") != std::string::npos) return "assets/models/stone.glb";
    if (lowerName.find("tree_and_bush") != std::string::npos || lowerName.find("trees_and_bush") != std::string::npos) return "assets/models/trees_and_bush_pack.glb";
    if (lowerName.find("bush") != std::string::npos) return "assets/models/bush.glb";
    if (lowerName.find("flatrock") != std::string::npos || lowerName.find("rock") != std::string::npos || lowerName.find("boulder") != std::string::npos) return "assets/models/rock.glb";
    if (lowerName.find("helmet") != std::string::npos || lowerName.find("relic") != std::string::npos) return "assets/models/DamagedHelmet.glb";
    if (lowerName.find("fox") != std::string::npos) return "assets/models/Fox.glb";
    if (lowerName.find("duck") != std::string::npos) return "assets/models/Duck.glb";
    if (lowerName.find("avocado") != std::string::npos) return "assets/models/Avocado.glb";
    if (lowerName.find("lantern") != std::string::npos) return "assets/models/Lantern.glb";
    if (lowerName.find("cesium") != std::string::npos) return "assets/models/CesiumMan.glb";
    if (lowerName.find("box") != std::string::npos && lowerName.find("player") == std::string::npos) return "assets/models/box.obj";
    if (lowerName.find("ipsc_metric") != std::string::npos || lowerName.find("metric_target") != std::string::npos) return "assets/models/IPSC_Metric_Target.obj";
    if (lowerName.find("ipsc_target_mini") != std::string::npos || lowerName.find("target_mini") != std::string::npos) return "assets/models/IPSC_Target_Mini.obj";
    if (lowerName.find("ipsc") != std::string::npos) return "assets/models/IPSC_Target.obj";
    if (lowerName.find("pepper_popper_mini") != std::string::npos || lowerName.find("popper_mini") != std::string::npos) return "assets/models/Pepper_Popper_Mini.obj";
    if (lowerName.find("pepper_popper") != std::string::npos) return "assets/models/Pepper_Popper.obj";
    if (lowerName.find("plate_a") != std::string::npos) return "assets/models/Plate_A.obj";
    if (lowerName.find("plate_b") != std::string::npos) return "assets/models/Plate_B.obj";
    if (lowerName.find("plate_c") != std::string::npos) return "assets/models/Plate_C.obj";
    if (lowerName.find("plate_d") != std::string::npos) return "assets/models/Plate_D.obj";
    if (lowerName.find("popper") != std::string::npos) return "assets/models/Popper.obj";
    if (lowerName.find("barricade") != std::string::npos || lowerName.find("wall_barricade") != std::string::npos) return "assets/models/Wall_Barricade.obj";
    if (lowerName.find("shootingtarget") != std::string::npos || lowerName.find("target") != std::string::npos) return "assets/models/ShootingTarget.obj";
    if (lowerName.find("warehouse") != std::string::npos) return "assets/models/warehouse.glb";

    // Dynamic search in assets/models and assets/
    std::vector<std::string> searchDirs = { "assets/models", "assets", "build/Release/assets/models" };
    for (const auto& dir : searchDirs)
    {
        if (!std::filesystem::exists(dir)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(dir))
        {
            if (entry.is_regular_file())
            {
                std::string stem = entry.path().stem().string();
                std::string ext = entry.path().extension().string();
                for (auto& c : ext) c = static_cast<char>(tolower(c));
                if (ext == ".glb" || ext == ".gltf" || ext == ".obj" || ext == ".fbx")
                {
                    std::string lowerStem = stem;
                    for (auto& c : lowerStem) c = static_cast<char>(tolower(c));
                    if (!lowerStem.empty() && lowerName.find(lowerStem) != std::string::npos)
                    {
                        return entry.path().string();
                    }
                }
            }
        }
    }
    return "";
}

void Scene::captureCameraSettings(const GizmoSystem* gizmo, const VulkanRenderer* renderer)
{
    if (gizmo)
    {
        cameraSettings.sceneRotationX = gizmo->sceneRotationX;
        cameraSettings.sceneRotationY = gizmo->sceneRotationY;
        cameraSettings.sceneCameraDistance = gizmo->sceneCameraDistance;
        cameraSettings.sceneCameraTarget = gizmo->sceneCameraTarget;

        cameraSettings.mainCameraPos = gizmo->mainCameraPos;
        cameraSettings.mainCameraTarget = gizmo->mainCameraTarget;
        cameraSettings.mainCameraFov = gizmo->mainCameraFov;
        cameraSettings.mainCameraNear = gizmo->mainCameraNear;
        cameraSettings.mainCameraFar = gizmo->mainCameraFar;
        cameraSettings.aspectRatioMode = static_cast<int>(gizmo->gameAspectRatioMode);

        cameraSettings.photoFrameWidthMeters = gizmo->photoFrameWidthMeters;
        cameraSettings.photoFrameHeightMeters = gizmo->photoFrameHeightMeters;
        cameraSettings.showPhotoGrid = gizmo->showPhotoGrid;
        cameraSettings.showPhotoFrameRulers = gizmo->showPhotoFrameRulers;

        cameraSettings.hasSavedCamera = true;
    }
    if (renderer)
    {
        cameraSettings.enableShadows = renderer->isShadowMappingEnabled();
    }
}

void Scene::applyCameraSettings(GizmoSystem* gizmo, VulkanRenderer* renderer)
{
    if (gizmo && cameraSettings.hasSavedCamera)
    {
        gizmo->sceneRotationX = cameraSettings.sceneRotationX;
        gizmo->sceneRotationY = cameraSettings.sceneRotationY;
        gizmo->sceneCameraDistance = cameraSettings.sceneCameraDistance;
        gizmo->sceneCameraTarget = cameraSettings.sceneCameraTarget;

        gizmo->mainCameraPos = cameraSettings.mainCameraPos;
        gizmo->mainCameraTarget = cameraSettings.mainCameraTarget;
        gizmo->mainCameraFov = cameraSettings.mainCameraFov;
        gizmo->mainCameraNear = cameraSettings.mainCameraNear;
        gizmo->mainCameraFar = cameraSettings.mainCameraFar;
        gizmo->gameAspectRatioMode = static_cast<GameViewAspectRatioMode>(cameraSettings.aspectRatioMode);

        gizmo->photoFrameWidthMeters = cameraSettings.photoFrameWidthMeters;
        gizmo->photoFrameHeightMeters = cameraSettings.photoFrameHeightMeters;
        gizmo->showPhotoGrid = cameraSettings.showPhotoGrid;
        gizmo->showPhotoFrameRulers = cameraSettings.showPhotoFrameRulers;
    }
    if (renderer)
    {
        renderer->setShadowMappingEnabled(cameraSettings.enableShadows);
    }
}

std::string Scene::getEnvironmentFileName(const std::string& envName)
{
    if (envName.empty()) return "scene.json";
    std::string slug = envName;
    for (auto& c : slug)
    {
        if (c == ' ' || c == '-' || c == '/' || c == '\\') c = '_';
        else c = static_cast<char>(tolower(c));
    }
    std::string cleanSlug = "";
    for (char c : slug)
    {
        if (isalnum(static_cast<unsigned char>(c)) || c == '_') cleanSlug += c;
    }
    std::string finalSlug = "";
    bool lastUnderscore = false;
    for (char c : cleanSlug)
    {
        if (c == '_') {
            if (!lastUnderscore) finalSlug += c;
            lastUnderscore = true;
        } else {
            finalSlug += c;
            lastUnderscore = false;
        }
    }
    if (finalSlug.empty()) return "scene.json";
    if (finalSlug.rfind("scene_", 0) == 0) return finalSlug + ".json";
    return "scene_" + finalSlug + ".json";
}

void Scene::saveScene(const std::string& filename, const AssetManager* assetManager, const GizmoSystem* gizmo, const VulkanRenderer* renderer)
{
    currentScenePath = filename;
    std::ofstream out(filename);
    if (!out.is_open())
    {
        printf("[Scene] Failed to open scene file for writing: %s\n", filename.c_str());
        return;
    }

    if (gizmo)
    {
        captureCameraSettings(gizmo, renderer);
    }

    out << "# ShapeRenderer Scene File\n";
    if (!currentEnvironmentName.empty())
    {
        out << "environment: " << currentEnvironmentName << "\n";
    }
    if (cameraSettings.hasSavedCamera)
    {
        out << "# Camera Settings\n";
        out << "camera_scene: " 
            << cameraSettings.sceneCameraTarget.x << " " 
            << cameraSettings.sceneCameraTarget.y << " " 
            << cameraSettings.sceneCameraTarget.z << " " 
            << cameraSettings.sceneRotationX << " " 
            << cameraSettings.sceneRotationY << " " 
            << cameraSettings.sceneCameraDistance << "\n";

        out << "camera_main: " 
            << cameraSettings.mainCameraPos.x << " " 
            << cameraSettings.mainCameraPos.y << " " 
            << cameraSettings.mainCameraPos.z << " " 
            << cameraSettings.mainCameraTarget.x << " " 
            << cameraSettings.mainCameraTarget.y << " " 
            << cameraSettings.mainCameraTarget.z << " " 
            << cameraSettings.mainCameraFov << " " 
            << cameraSettings.mainCameraNear << " " 
            << cameraSettings.mainCameraFar << " " 
            << cameraSettings.aspectRatioMode << "\n";

        out << "camera_photo_frame: "
            << cameraSettings.photoFrameWidthMeters << " "
            << cameraSettings.photoFrameHeightMeters << " "
            << (cameraSettings.showPhotoGrid ? 1 : 0) << " "
            << (cameraSettings.showPhotoFrameRulers ? 1 : 0) << "\n";
    }

    out << "scene_settings: "
        << selectedObjectIndex << " "
        << (cameraSettings.enableShadows ? 1 : 0) << " "
        << playerStartPos.x << " " << playerStartPos.y << " " << playerStartPos.z << "\n\n";

    for (size_t i = 0; i < sceneObjects.size(); ++i)
    {
        const auto& obj = sceneObjects[i];
        out << "object: " << obj.name << "\n";
        out << "id: " << (obj.id >= 0 ? obj.id : static_cast<int>(i)) << "\n";
        out << "parentId: " << obj.parentId << "\n";
        out << "type: " << static_cast<int>(obj.type) << "\n";
        out << "position: " << obj.position.x << " " << obj.position.y << " " << obj.position.z << "\n";
        out << "rotation: " << obj.rotation.x << " " << obj.rotation.y << " " << obj.rotation.z << "\n";
        out << "scale: " << obj.scale.x << " " << obj.scale.y << " " << obj.scale.z << "\n";
        out << "color: " << obj.color.r << " " << obj.color.g << " " << obj.color.b << " " << obj.color.a << "\n";

        // Determine Mesh Source
        std::string mPath = obj.modelPath;
        if (mPath.empty() && assetManager && obj.meshId >= 0)
        {
            mPath = assetManager->getModelPath(obj.meshId);
        }
        for (auto& c : mPath) if (c == '\\') c = '/';

        if (mPath.rfind("terrain:", 0) == 0)
        {
            // Format: terrain:w:d:cellSize:heightScale:seed:biome
            std::stringstream ss(mPath.substr(8));
            std::string w, d, cs, hs, seed, biome;
            std::getline(ss, w, ':');
            std::getline(ss, d, ':');
            std::getline(ss, cs, ':');
            std::getline(ss, hs, ':');
            std::getline(ss, seed, ':');
            std::getline(ss, biome, ':');
            out << "terrainMesh: " << w << " " << d << " " << cs << " " << hs << " " << seed << " " << biome << "\n";
        }
        else if (!mPath.empty() && mPath.rfind("primitive://", 0) != 0)
        {
            out << "model: " << mPath << "\n";
        }
        else if (assetManager)
        {
            if (obj.meshId == assetManager->getCubeMeshId()) out << "primitiveMesh: CUBE\n";
            else if (obj.meshId == assetManager->getSphereMeshId()) out << "primitiveMesh: SPHERE\n";
            else if (obj.meshId == assetManager->getPlaneMeshId()) out << "primitiveMesh: PLANE\n";
            else if (obj.meshId == assetManager->getCylinderMeshId()) out << "primitiveMesh: CYLINDER\n";
            else if (obj.meshId == assetManager->getConeMeshId()) out << "primitiveMesh: CONE\n";
            else if (obj.meshId < 0) out << "primitiveMesh: NONE\n";
            else if (obj.name.find("Terrain") != std::string::npos || obj.name.find("Landscape") != std::string::npos)
            {
                out << "terrainMesh: 35 35 1.0 5.0 42 0\n";
            }
            else
            {
                out << "primitiveMesh: CUBE\n";
            }
        }
        else
        {
            if (obj.type == ObjectType::SPHERE) out << "primitiveMesh: SPHERE\n";
            else if (obj.type == ObjectType::PLANE) out << "primitiveMesh: PLANE\n";
            else out << "primitiveMesh: CUBE\n";
        }

        // Texture Path
        std::string tPath = obj.texturePath;
        if (tPath.empty() && assetManager && obj.textureId >= 0)
        {
            tPath = assetManager->getTexturePath(obj.textureId);
        }
        if (tPath.empty() && (obj.name.find("Warehouse") != std::string::npos || mPath.find("warehouse") != std::string::npos))
        {
            if (std::filesystem::exists("assets/models/warehouse.png")) tPath = "assets/models/warehouse.png";
            else if (std::filesystem::exists("assets/textures/warehouse.png")) tPath = "assets/textures/warehouse.png";
        }
        for (auto& c : tPath) if (c == '\\') c = '/';

        if (!tPath.empty())
        {
            out << "texture: " << tPath << "\n";
        }
        else if (obj.textureId < 0)
        {
            out << "texture: none\n";
        }

        // PBR Material Properties
        out << "pbr: " << (obj.usePBR ? 1 : 0) << " " << obj.roughness << " " << obj.metallic << " " << obj.ambientOcclusion << "\n";

        // Physics & RigidBody
        out << "physics: " << (obj.isPhysicsEnabled ? 1 : 0) << "\n";
        auto rbSave = obj.getComponent<RigidBodyComponent>();
        if (rbSave)
        {
            out << "hasRigidBody: 1\n";
            out << "mass: " << rbSave->mass << "\n";
            out << "friction: " << rbSave->friction << "\n";
            out << "restitution: " << rbSave->restitution << "\n";
            out << "linearDrag: " << rbSave->linearDrag << "\n";
            out << "angularDrag: " << rbSave->angularDrag << "\n";
            out << "colliderType: " << static_cast<int>(rbSave->colliderType) << "\n";
            out << "motionType: " << static_cast<int>(rbSave->motionType) << "\n";
            out << "useGravity: " << (rbSave->useGravity ? 1 : 0) << "\n";
            out << "isTrigger: " << (rbSave->isTrigger ? 1 : 0) << "\n";
            out << "velocity: " << obj.velocity.x << " " << obj.velocity.y << " " << obj.velocity.z << "\n";
            out << "angularVelocity: " << rbSave->angularVelocity.x << " " << rbSave->angularVelocity.y << " " << rbSave->angularVelocity.z << "\n";
        }
        else
        {
            out << "hasRigidBody: 0\n";
        }

        // Light Component
        auto lightComp = obj.getComponent<LightComponent>();
        if (lightComp || obj.type == ObjectType::LIGHT)
        {
            float intensity = lightComp ? lightComp->intensity : 1.0f;
            glm::vec4 lColor = lightComp ? lightComp->color : obj.color;
            out << "light: " << lColor.r << " " << lColor.g << " " << lColor.b << " " << lColor.a << " " << intensity << "\n";
        }

        // MeshRenderer Component
        auto mrComp = obj.getComponent<MeshRendererComponent>();
        if (mrComp)
        {
            out << "meshRenderer: " << (mrComp->visible ? 1 : 0) << "\n";
        }

        // Lua Scripts
        out << "luaScriptsCount: " << obj.luaScripts.size() << "\n";
        for (const auto& script : obj.luaScripts)
        {
            bool hasTrailingNl = (!script.empty() && script.back() == '\n');
            out << "luaScriptStart: " << (hasTrailingNl ? 1 : 0) << "\n";
            out << script;
            if (!hasTrailingNl) out << "\n";
            out << "luaScriptEnd:\n";
        }
        out << "\n";
    }

    out.close();
    printf("[Scene] Saved scene successfully to %s (%zu objects)\n", filename.c_str(), sceneObjects.size());
}

bool Scene::exportToJMonkeyEngine(const std::string& filename, const AssetManager* assetManager, const GizmoSystem* gizmo, const VulkanRenderer* renderer)
{
    if (gizmo)
    {
        captureCameraSettings(gizmo, renderer);
    }

    auto escapeJson = [](const std::string& s) -> std::string {
        std::string res;
        for (char c : s) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "/";
            else if (c == '\b') res += "\\b";
            else if (c == '\f') res += "\\f";
            else if (c == '\n') res += "\\n";
            else if (c == '\r') res += "\\r";
            else if (c == '\t') res += "\\t";
            else res += c;
        }
        return res;
    };

    auto normalizeAssetPath = [](const std::string& raw) -> std::string {
        std::string p = raw;
        for (auto& c : p) if (c == '\\') c = '/';
        if (p.rfind("./", 0) == 0) p = p.substr(2);
        if (p.rfind("assets/", 0) == 0) p = p.substr(7);
        return p;
    };

    // 1. Terrain export (OBJ & Heightmap PNG)
    bool hasTerrain = false;
    std::string terrainName = "Terrain";
    int terrainW = 35, terrainD = 35, terrainSeed = 42, terrainBiome = 0;
    float terrainCell = 1.0f, terrainHeight = 2.5f;
    std::string terrainTex = "grass.jpg";

    for (const auto& obj : sceneObjects)
    {
        std::string mPath = obj.modelPath;
        if (mPath.empty() && assetManager && obj.meshId >= 0)
        {
            mPath = assetManager->getModelPath(obj.meshId);
        }
        bool isTerrainObj = (mPath.rfind("terrain:", 0) == 0 ||
                             obj.name.find("Terrain") != std::string::npos ||
                             obj.name.find("Landscape") != std::string::npos ||
                             obj.name.find("Dunes") != std::string::npos ||
                             obj.name.find("Valley") != std::string::npos);

        if (isTerrainObj)
        {
            hasTerrain = true;
            terrainName = obj.name;
            if (!obj.texturePath.empty()) terrainTex = normalizeAssetPath(obj.texturePath);

            if (mPath.rfind("terrain:", 0) == 0)
            {
                std::stringstream ss(mPath.substr(8));
                std::string sw, sd, scs, shs, sseed, sbiome;
                std::getline(ss, sw, ':');
                std::getline(ss, sd, ':');
                std::getline(ss, scs, ':');
                std::getline(ss, shs, ':');
                std::getline(ss, sseed, ':');
                std::getline(ss, sbiome, ':');
                try {
                    if (!sw.empty()) terrainW = std::stoi(sw);
                    if (!sd.empty()) terrainD = std::stoi(sd);
                    if (!scs.empty()) terrainCell = std::stof(scs);
                    if (!shs.empty()) terrainHeight = std::stof(shs);
                    if (!sseed.empty()) terrainSeed = std::stoi(sseed);
                    if (!sbiome.empty()) terrainBiome = std::stoi(sbiome);
                } catch (...) {}
            }

            if (assetManager && obj.meshId >= 0)
            {
                const Mesh* tm = assetManager->getMesh(obj.meshId);
                if (tm && !tm->vertices.empty())
                {
                    std::error_code ec;
                    std::filesystem::create_directories("assets/models", ec);
                    std::filesystem::create_directories("assets/textures", ec);

                    // Write Wavefront OBJ
                    std::string terrainObjPath = "assets/models/terrain.obj";
                    std::ofstream ofs(terrainObjPath);
                    if (ofs.is_open())
                    {
                        ofs << "# Terrain Wavefront OBJ Exported for jMonkeyEngine\n";
                        ofs << "o Terrain\n";
                        for (const auto& v : tm->vertices)
                        {
                            ofs << "v " << v.pos.x << " " << v.pos.y << " " << v.pos.z << "\n";
                        }
                        for (const auto& v : tm->vertices)
                        {
                            ofs << "vt " << v.texCoord.x << " " << (1.0f - v.texCoord.y) << "\n";
                        }
                        for (const auto& v : tm->vertices)
                        {
                            ofs << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
                        }
                        for (size_t idx = 0; idx + 2 < tm->indices.size(); idx += 3)
                        {
                            uint32_t i1 = tm->indices[idx] + 1;
                            uint32_t i2 = tm->indices[idx + 1] + 1;
                            uint32_t i3 = tm->indices[idx + 2] + 1;
                            ofs << "f " << i1 << "/" << i1 << "/" << i1 << " "
                                << i2 << "/" << i2 << "/" << i2 << " "
                                << i3 << "/" << i3 << "/" << i3 << "\n";
                        }
                        ofs.close();
                        printf("[jME Export] Exported terrain mesh to %s (%zu vertices, %zu triangles)\n",
                               terrainObjPath.c_str(), tm->vertices.size(), tm->indices.size() / 3);
                    }

                    // Write Heightmap PNG
                    float minY = 1e9f, maxY = -1e9f;
                    for (const auto& v : tm->vertices)
                    {
                        if (v.pos.y < minY) minY = v.pos.y;
                        if (v.pos.y > maxY) maxY = v.pos.y;
                    }
                    float rangeY = (maxY - minY > 0.001f) ? (maxY - minY) : 1.0f;
                    std::vector<unsigned char> hmPixels(terrainW * terrainD, 0);
                    for (int z = 0; z < terrainD; ++z)
                    {
                        for (int x = 0; x < terrainW; ++x)
                        {
                            int vIdx = z * terrainW + x;
                            if (vIdx < static_cast<int>(tm->vertices.size()))
                            {
                                float normH = (tm->vertices[vIdx].pos.y - minY) / rangeY;
                                normH = std::clamp(normH, 0.0f, 1.0f);
                                hmPixels[z * terrainW + x] = static_cast<unsigned char>(normH * 255.0f);
                            }
                        }
                    }
                    std::string hmPath = "assets/textures/terrain_heightmap.png";
                    stbi_write_png(hmPath.c_str(), terrainW, terrainD, 1, hmPixels.data(), terrainW);
                    printf("[jME Export] Exported terrain heightmap image to %s (%dx%d)\n", hmPath.c_str(), terrainW, terrainD);
                }
            }
            break;
        }
    }

    // 2. Find Directional Light
    glm::vec3 sunDir(-0.35f, -0.85f, -0.4f);
    glm::vec4 sunColor(1.0f, 0.95f, 0.85f, 1.0f);
    float sunIntensity = 1.6f;

    for (const auto& obj : sceneObjects)
    {
        if (obj.type == ObjectType::LIGHT || obj.hasComponent(ComponentType::LIGHT))
        {
            if (glm::length(obj.position) > 0.001f)
            {
                sunDir = -glm::normalize(obj.position);
            }
            sunColor = obj.color;
            if (auto lc = obj.getComponent<LightComponent>())
            {
                sunIntensity = lc->intensity;
                sunColor = lc->color;
            }
            break;
        }
    }

    // 3. Write Standard RFC 8259 JSON
    std::ofstream jsonFile(filename);
    if (!jsonFile.is_open())
    {
        printf("[jME Export] Failed to open %s for writing!\n", filename.c_str());
        return false;
    }

    jsonFile << "{\n";
    jsonFile << "  \"format\": \"jMonkeyEngine Scene Descriptor\",\n";
    jsonFile << "  \"version\": \"1.0\",\n";
    jsonFile << "  \"engine\": \"Unity Hub 3D Vulkan Engine\",\n";
    jsonFile << "  \"environment\": \"" << escapeJson(currentEnvironmentName.empty() ? "Default" : currentEnvironmentName) << "\",\n";

    // Camera
    jsonFile << "  \"camera\": {\n";
    jsonFile << "    \"position\": [" << cameraSettings.mainCameraPos.x << ", " << cameraSettings.mainCameraPos.y << ", " << cameraSettings.mainCameraPos.z << "],\n";
    jsonFile << "    \"target\": [" << cameraSettings.mainCameraTarget.x << ", " << cameraSettings.mainCameraTarget.y << ", " << cameraSettings.mainCameraTarget.z << "],\n";
    jsonFile << "    \"fov\": " << cameraSettings.mainCameraFov << ",\n";
    jsonFile << "    \"near\": " << cameraSettings.mainCameraNear << ",\n";
    jsonFile << "    \"far\": " << cameraSettings.mainCameraFar << "\n";
    jsonFile << "  },\n";

    // Sun Light
    jsonFile << "  \"sunLight\": {\n";
    jsonFile << "    \"direction\": [" << sunDir.x << ", " << sunDir.y << ", " << sunDir.z << "],\n";
    jsonFile << "    \"color\": [" << sunColor.r << ", " << sunColor.g << ", " << sunColor.b << ", " << sunColor.a << "],\n";
    jsonFile << "    \"intensity\": " << sunIntensity << "\n";
    jsonFile << "  },\n";

    // Terrain Info
    if (hasTerrain)
    {
        jsonFile << "  \"terrain\": {\n";
        jsonFile << "    \"name\": \"" << escapeJson(terrainName) << "\",\n";
        jsonFile << "    \"modelPath\": \"models/terrain.obj\",\n";
        jsonFile << "    \"heightmapPath\": \"textures/terrain_heightmap.png\",\n";
        jsonFile << "    \"texturePath\": \"" << escapeJson(terrainTex) << "\",\n";
        jsonFile << "    \"gridW\": " << terrainW << ",\n";
        jsonFile << "    \"gridD\": " << terrainD << ",\n";
        jsonFile << "    \"cellSize\": " << terrainCell << ",\n";
        jsonFile << "    \"heightScale\": " << terrainHeight << ",\n";
        jsonFile << "    \"seed\": " << terrainSeed << ",\n";
        jsonFile << "    \"biome\": " << terrainBiome << "\n";
        jsonFile << "  },\n";
    }

    // Objects
    jsonFile << "  \"objects\": [\n";

    bool firstItem = true;
    for (size_t i = 0; i < sceneObjects.size(); ++i)
    {
        const auto& obj = sceneObjects[i];
        if (obj.type == ObjectType::LIGHT) continue; // Handled in sunLight

        if (!firstItem) jsonFile << ",\n";
        firstItem = false;

        // Determine Model Path / Primitive Type
        std::string mPath = obj.modelPath;
        if (mPath.empty() && assetManager && obj.meshId >= 0)
        {
            mPath = assetManager->getModelPath(obj.meshId);
        }
        for (auto& c : mPath) if (c == '\\') c = '/';

        bool isTerrainObj = (mPath.rfind("terrain:", 0) == 0 ||
                             obj.name.find("Terrain") != std::string::npos ||
                             obj.name.find("Landscape") != std::string::npos ||
                             obj.name.find("Dunes") != std::string::npos ||
                             obj.name.find("Valley") != std::string::npos);

        std::string objType = "model";
        std::string primType = "NONE";
        std::string jmeModelPath = "";

        if (isTerrainObj)
        {
            objType = "terrain";
            jmeModelPath = "models/terrain.obj";
        }
        else if (!mPath.empty() && mPath.rfind("primitive://", 0) != 0)
        {
            objType = "model";
            jmeModelPath = normalizeAssetPath(mPath);
        }
        else if (assetManager)
        {
            objType = "primitive";
            if (obj.meshId == assetManager->getCubeMeshId()) primType = "CUBE";
            else if (obj.meshId == assetManager->getSphereMeshId()) primType = "SPHERE";
            else if (obj.meshId == assetManager->getPlaneMeshId()) primType = "PLANE";
            else if (obj.meshId == assetManager->getCylinderMeshId()) primType = "CYLINDER";
            else if (obj.meshId == assetManager->getConeMeshId()) primType = "CONE";
            else primType = "CUBE";
        }
        else
        {
            objType = "primitive";
            if (obj.type == ObjectType::SPHERE) primType = "SPHERE";
            else if (obj.type == ObjectType::PLANE) primType = "PLANE";
            else primType = "CUBE";
        }

        // Texture Path
        std::string tPath = obj.texturePath;
        if (tPath.empty() && assetManager)
        {
            if (obj.textureId >= 0)
            {
                tPath = assetManager->getTexturePath(obj.textureId);
            }
            else if (obj.meshId >= 0)
            {
                const Mesh* m = assetManager->getMesh(obj.meshId);
                if (m && m->defaultTextureId >= 0)
                {
                    tPath = assetManager->getTexturePath(m->defaultTextureId);
                }
            }
        }
        std::string jmeTexPath = normalizeAssetPath(tPath);

        // Quaternion Rotation
        glm::quat qZ = glm::angleAxis(glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::quat qY = glm::angleAxis(glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat qX = glm::angleAxis(glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat q = qZ * qY * qX;

        // RigidBody
        auto rb = obj.getComponent<RigidBodyComponent>();
        bool hasRb = (rb != nullptr);
        float mass = rb ? rb->mass : 0.0f;
        float friction = rb ? rb->friction : 0.5f;
        float restitution = rb ? rb->restitution : 0.0f;
        std::string colType = "mesh";
        if (rb)
        {
            if (rb->colliderType == ColliderType::BOX) colType = "box";
            else if (rb->colliderType == ColliderType::SPHERE) colType = "sphere";
            else if (rb->colliderType == ColliderType::CAPSULE) colType = "capsule";
            else if (rb->colliderType == ColliderType::PLANE) colType = "plane";
        }
        else if (objType == "primitive")
        {
            if (primType == "SPHERE") colType = "sphere";
            else colType = "box";
        }

        jsonFile << "    {\n";
        jsonFile << "      \"id\": " << (obj.id >= 0 ? obj.id : static_cast<int>(i)) << ",\n";
        jsonFile << "      \"parentId\": " << obj.parentId << ",\n";
        jsonFile << "      \"name\": \"" << escapeJson(obj.name) << "\",\n";
        jsonFile << "      \"type\": \"" << objType << "\",\n";
        if (objType == "primitive")
        {
            jsonFile << "      \"primitiveType\": \"" << primType << "\",\n";
        }
        else
        {
            jsonFile << "      \"modelPath\": \"" << escapeJson(jmeModelPath) << "\",\n";
            jsonFile << "      \"rawModelPath\": \"" << escapeJson(mPath) << "\",\n";
        }

        if (assetManager && obj.meshId >= 0)
        {
            const Mesh* m = assetManager->getMesh(obj.meshId);
            if (m)
            {
                jsonFile << "      \"modelOffset\": [" << m->pivotOffset.x << ", " << m->pivotOffset.y << ", " << m->pivotOffset.z << "],\n";
                jsonFile << "      \"modelScale\": " << m->targetScale << ",\n";
            }
        }

        jsonFile << "      \"position\": [" << obj.position.x << ", " << obj.position.y << ", " << obj.position.z << "],\n";
        jsonFile << "      \"rotationEuler\": [" << obj.rotation.x << ", " << obj.rotation.y << ", " << obj.rotation.z << "],\n";
        jsonFile << "      \"rotationQuat\": [" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << "],\n";
        jsonFile << "      \"scale\": [" << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << "],\n";

        if (obj.parentId >= 0)
        {
            glm::mat4 wMat = getWorldMatrix(static_cast<int>(i));
            glm::vec3 wPos = glm::vec3(wMat[3]);
            glm::vec3 wScale(glm::length(glm::vec3(wMat[0])), glm::length(glm::vec3(wMat[1])), glm::length(glm::vec3(wMat[2])));
            glm::mat3 rotM(glm::vec3(wMat[0]) / (wScale.x > 0.0001f ? wScale.x : 1.0f),
                           glm::vec3(wMat[1]) / (wScale.y > 0.0001f ? wScale.y : 1.0f),
                           glm::vec3(wMat[2]) / (wScale.z > 0.0001f ? wScale.z : 1.0f));
            glm::quat wQuat = glm::quat_cast(rotM);
            jsonFile << "      \"worldPosition\": [" << wPos.x << ", " << wPos.y << ", " << wPos.z << "],\n";
            jsonFile << "      \"worldRotationQuat\": [" << wQuat.x << ", " << wQuat.y << ", " << wQuat.z << ", " << wQuat.w << "],\n";
            jsonFile << "      \"worldScale\": [" << wScale.x << ", " << wScale.y << ", " << wScale.z << "],\n";
        }

        jsonFile << "      \"color\": [" << obj.color.r << ", " << obj.color.g << ", " << obj.color.b << ", " << obj.color.a << "],\n";
        jsonFile << "      \"material\": {\n";
        jsonFile << "        \"texturePath\": \"" << escapeJson(jmeTexPath) << "\",\n";
        jsonFile << "        \"pbr\": " << (obj.usePBR ? "true" : "false") << ",\n";
        jsonFile << "        \"roughness\": " << obj.roughness << ",\n";
        jsonFile << "        \"metallic\": " << obj.metallic << ",\n";
        jsonFile << "        \"ambientOcclusion\": " << obj.ambientOcclusion << "\n";
        jsonFile << "      },\n";

        if (assetManager && obj.meshId >= 0)
        {
            const Mesh* m = assetManager->getMesh(obj.meshId);
            if (m && !m->submeshes.empty())
            {
                jsonFile << "      \"submeshes\": [\n";
                for (size_t smIdx = 0; smIdx < m->submeshes.size(); ++smIdx)
                {
                    const auto& sm = m->submeshes[smIdx];
                    int smTexId = sm.textureId;
                    if (obj.textureId >= 0 && obj.textureId != m->defaultTextureId)
                    {
                        smTexId = obj.textureId;
                    }
                    std::string smTexPath = "";
                    if (smTexId >= 0)
                    {
                        smTexPath = normalizeAssetPath(assetManager->getTexturePath(smTexId));
                    }
                    jsonFile << "        {\n";
                    jsonFile << "          \"index\": " << smIdx << ",\n";
                    jsonFile << "          \"texturePath\": \"" << escapeJson(smTexPath) << "\",\n";
                    jsonFile << "          \"roughness\": " << sm.roughness << ",\n";
                    jsonFile << "          \"metallic\": " << sm.metallic << ",\n";
                    jsonFile << "          \"isFoliage\": " << (sm.isFoliage ? "true" : "false") << ",\n";
                    jsonFile << "          \"twoSided\": " << (sm.twoSided ? "true" : "false") << "\n";
                    jsonFile << "        }" << (smIdx + 1 < m->submeshes.size() ? "," : "") << "\n";
                }
                jsonFile << "      ],\n";
            }
        }

        jsonFile << "      \"physics\": {\n";
        jsonFile << "        \"enabled\": " << (obj.isPhysicsEnabled ? "true" : "false") << ",\n";
        jsonFile << "        \"hasRigidBody\": " << (hasRb ? "true" : "false") << ",\n";
        jsonFile << "        \"mass\": " << mass << ",\n";
        jsonFile << "        \"colliderType\": \"" << colType << "\",\n";
        jsonFile << "        \"friction\": " << friction << ",\n";
        jsonFile << "        \"restitution\": " << restitution << "\n";
        jsonFile << "      }\n";
        jsonFile << "    }";
    }

    jsonFile << "\n  ]\n";
    jsonFile << "}\n";
    jsonFile.close();

    printf("[jME Export] Scene successfully exported to %s!\n", filename.c_str());

    // Also copy to build/Release/ if it exists so both locations have it
    if (filename == "scene_jme.json" && std::filesystem::exists("build/Release"))
    {
        std::error_code ec;
        std::filesystem::copy_file("scene_jme.json", "build/Release/scene_jme.json", std::filesystem::copy_options::overwrite_existing, ec);
    }

    // Sync to Jme_scene directory structure
    std::vector<std::string> jmeSceneDirs = {
        "../Jme_scene",
        "../../Jme_scene",
        "D:/Game_prj/Jme_scene"
    };

    std::string canonicalExportFile = std::filesystem::absolute(filename).string();

    for (const auto& jDir : jmeSceneDirs)
    {
        if (std::filesystem::exists(jDir) && std::filesystem::is_directory(jDir))
        {
            std::error_code ec;
            std::string jmeSceneFile = jDir + "/scene.json";
            std::filesystem::copy_file(filename, jmeSceneFile, std::filesystem::copy_options::overwrite_existing, ec);

            std::string jmeResDir = jDir + "/src/main/resources";
            if (std::filesystem::exists(jmeResDir))
            {
                std::filesystem::copy_file(filename, jmeResDir + "/scene.json", std::filesystem::copy_options::overwrite_existing, ec);
            }

            // Update last_scene.txt
            std::ofstream lastSceneOfs(jDir + "/last_scene.txt");
            if (lastSceneOfs.is_open())
            {
                lastSceneOfs << canonicalExportFile << "\n";
                lastSceneOfs.close();
            }

            // Sync terrain assets if generated
            if (hasTerrain)
            {
                std::filesystem::create_directories(jDir + "/assets/models", ec);
                std::filesystem::create_directories(jDir + "/models", ec);
                std::filesystem::create_directories(jDir + "/assets/textures", ec);
                std::filesystem::create_directories(jDir + "/textures", ec);

                if (std::filesystem::exists("assets/models/terrain.obj"))
                {
                    std::filesystem::copy_file("assets/models/terrain.obj", jDir + "/assets/models/terrain.obj", std::filesystem::copy_options::overwrite_existing, ec);
                    std::filesystem::copy_file("assets/models/terrain.obj", jDir + "/models/terrain.obj", std::filesystem::copy_options::overwrite_existing, ec);
                }
                if (std::filesystem::exists("assets/textures/terrain_heightmap.png"))
                {
                    std::filesystem::copy_file("assets/textures/terrain_heightmap.png", jDir + "/assets/textures/terrain_heightmap.png", std::filesystem::copy_options::overwrite_existing, ec);
                    std::filesystem::copy_file("assets/textures/terrain_heightmap.png", jDir + "/textures/terrain_heightmap.png", std::filesystem::copy_options::overwrite_existing, ec);
                }
            }

            printf("[jME Export] Synced exported scene and assets to %s successfully!\n", jDir.c_str());
            break;
        }
    }

    // 4. Generate SceneLoader.java Helper File
    std::string javaLoaderPath = "SceneLoader.java";
    std::ofstream javaOfs(javaLoaderPath);
    if (javaOfs.is_open())
    {
        javaOfs << "package com.mygame;\n\n"
                << "import com.jme3.app.SimpleApplication;\n"
                << "import com.jme3.asset.AssetManager;\n"
                << "import com.jme3.bullet.BulletAppState;\n"
                << "import com.jme3.bullet.collision.shapes.CollisionShape;\n"
                << "import com.jme3.bullet.control.RigidBodyControl;\n"
                << "import com.jme3.bullet.util.CollisionShapeFactory;\n"
                << "import com.jme3.light.AmbientLight;\n"
                << "import com.jme3.light.DirectionalLight;\n"
                << "import com.jme3.material.Material;\n"
                << "import com.jme3.math.ColorRGBA;\n"
                << "import com.jme3.math.Quaternion;\n"
                << "import com.jme3.math.Vector3f;\n"
                << "import com.jme3.scene.Geometry;\n"
                << "import com.jme3.scene.Node;\n"
                << "import com.jme3.scene.Spatial;\n"
                << "import com.jme3.scene.shape.Box;\n"
                << "import com.jme3.scene.shape.Cylinder;\n"
                << "import com.jme3.scene.shape.Sphere;\n"
                << "import com.google.gson.*;\n\n"
                << "import java.io.File;\n"
                << "import java.io.FileInputStream;\n"
                << "import java.io.InputStreamReader;\n\n"
                << "/**\n"
                << " * SceneLoader - Nap canh xuat tu Unity Hub 3D Vulkan Engine vao jMonkeyEngine 3.x\n"
                << " */\n"
                << "public class SceneLoader {\n\n"
                << "    public static Node loadScene(String jsonFilePath, SimpleApplication app, BulletAppState bulletAppState) {\n"
                << "        Node sceneRoot = new Node(\"ImportedScene\");\n"
                << "        AssetManager assetManager = app.getAssetManager();\n\n"
                << "        try {\n"
                << "            File f = new File(jsonFilePath);\n"
                << "            if (!f.exists()) {\n"
                << "                System.err.println(\"[SceneLoader] File not found: \" + jsonFilePath);\n"
                << "                return sceneRoot;\n"
                << "            }\n\n"
                << "            JsonObject root = JsonParser.parseReader(new InputStreamReader(new FileInputStream(f), \"UTF-8\")).getAsJsonObject();\n"
                << "            System.out.println(\"[SceneLoader] Loading scene: \" + (root.has(\"environment\") ? root.get(\"environment\").getAsString() : \"Default\"));\n\n"
                << "            // 1. Sun Light\n"
                << "            if (root.has(\"sunLight\")) {\n"
                << "                JsonObject sun = root.getAsJsonObject(\"sunLight\");\n"
                << "                JsonArray dir = sun.getAsJsonArray(\"direction\");\n"
                << "                JsonArray col = sun.getAsJsonArray(\"color\");\n"
                << "                float intensity = sun.has(\"intensity\") ? sun.get(\"intensity\").getAsFloat() : 1.0f;\n\n"
                << "                DirectionalLight dl = new DirectionalLight();\n"
                << "                dl.setDirection(new Vector3f(dir.get(0).getAsFloat(), dir.get(1).getAsFloat(), dir.get(2).getAsFloat()).normalizeLocal());\n"
                << "                dl.setColor(new ColorRGBA(col.get(0).getAsFloat() * intensity, col.get(1).getAsFloat() * intensity, col.get(2).getAsFloat() * intensity, 1.0f));\n"
                << "                app.getRootNode().addLight(dl);\n\n"
                << "                AmbientLight al = new AmbientLight();\n"
                << "                al.setColor(new ColorRGBA(0.25f, 0.25f, 0.28f, 1.0f));\n"
                << "                app.getRootNode().addLight(al);\n"
                << "            }\n\n"
                << "            // 2. Camera Settings\n"
                << "            if (root.has(\"camera\")) {\n"
                << "                JsonObject camObj = root.getAsJsonObject(\"camera\");\n"
                << "                JsonArray pos = camObj.getAsJsonArray(\"position\");\n"
                << "                JsonArray target = camObj.getAsJsonArray(\"target\");\n"
                << "                app.getCamera().setLocation(new Vector3f(pos.get(0).getAsFloat(), pos.get(1).getAsFloat(), pos.get(2).getAsFloat()));\n"
                << "                app.getCamera().lookAt(new Vector3f(target.get(0).getAsFloat(), target.get(1).getAsFloat(), target.get(2).getAsFloat()), Vector3f.UNIT_Y);\n"
                << "            }\n\n"
                << "            // 3. Load Objects\n"
                << "            if (root.has(\"objects\")) {\n"
                << "                JsonArray objects = root.getAsJsonArray(\"objects\");\n"
                << "                for (JsonElement elem : objects) {\n"
                << "                    JsonObject obj = elem.getAsJsonObject();\n"
                << "                    String name = obj.get(\"name\").getAsString();\n"
                << "                    String type = obj.has(\"type\") ? obj.get(\"type\").getAsString() : \"model\";\n\n"
                << "                    Spatial spatial = null;\n"
                << "                    if (\"model\".equalsIgnoreCase(type) || \"terrain\".equalsIgnoreCase(type)) {\n"
                << "                        String modelPath = obj.has(\"modelPath\") ? obj.get(\"modelPath\").getAsString() : \"\";\n"
                << "                        if (!modelPath.isEmpty()) {\n"
                << "                            try {\n"
                << "                                spatial = assetManager.loadModel(modelPath);\n"
                << "                            } catch (Exception ex) {\n"
                << "                                System.err.println(\"[SceneLoader] Warning: Failed to load model '\" + modelPath + \"' (\" + ex.getMessage() + \")\");\n"
                << "                            }\n"
                << "                        }\n"
                << "                    } else if (\"primitive\".equalsIgnoreCase(type)) {\n"
                << "                        String primType = obj.has(\"primitiveType\") ? obj.get(\"primitiveType\").getAsString() : \"CUBE\";\n"
                << "                        Geometry geom = null;\n"
                << "                        if (\"SPHERE\".equalsIgnoreCase(primType)) {\n"
                << "                            geom = new Geometry(name, new Sphere(16, 16, 0.5f));\n"
                << "                        } else if (\"CYLINDER\".equalsIgnoreCase(primType)) {\n"
                << "                            geom = new Geometry(name, new Cylinder(16, 16, 0.5f, 1.0f, true));\n"
                << "                        } else {\n"
                << "                            geom = new Geometry(name, new Box(0.5f, 0.5f, 0.5f));\n"
                << "                        }\n"
                << "                        Material mat = new Material(assetManager, \"Common/MatDefs/Misc/Unshaded.j3md\");\n"
                << "                        if (obj.has(\"color\")) {\n"
                << "                            JsonArray col = obj.getAsJsonArray(\"color\");\n"
                << "                            mat.setColor(\"Color\", new ColorRGBA(col.get(0).getAsFloat(), col.get(1).getAsFloat(), col.get(2).getAsFloat(), col.get(3).getAsFloat()));\n"
                << "                        }\n"
                << "                        geom.setMaterial(mat);\n"
                << "                        spatial = geom;\n"
                << "                    }\n\n"
                << "                    if (spatial == null) continue;\n"
                << "                    spatial.setName(name);\n\n"
                << "                    // Position\n"
                << "                    if (obj.has(\"position\")) {\n"
                << "                        JsonArray p = obj.getAsJsonArray(\"position\");\n"
                << "                        spatial.setLocalTranslation(p.get(0).getAsFloat(), p.get(1).getAsFloat(), p.get(2).getAsFloat());\n"
                << "                    }\n\n"
                << "                    // Rotation (Quaternion)\n"
                << "                    if (obj.has(\"rotationQuat\")) {\n"
                << "                        JsonArray q = obj.getAsJsonArray(\"rotationQuat\");\n"
                << "                        spatial.setLocalRotation(new Quaternion(q.get(0).getAsFloat(), q.get(1).getAsFloat(), q.get(2).getAsFloat(), q.get(3).getAsFloat()));\n"
                << "                    }\n\n"
                << "                    // Scale\n"
                << "                    if (obj.has(\"scale\")) {\n"
                << "                        JsonArray s = obj.getAsJsonArray(\"scale\");\n"
                << "                        spatial.setLocalScale(s.get(0).getAsFloat(), s.get(1).getAsFloat(), s.get(2).getAsFloat());\n"
                << "                    }\n\n"
                << "                    // Physics\n"
                << "                    if (bulletAppState != null && obj.has(\"physics\")) {\n"
                << "                        JsonObject phys = obj.getAsJsonObject(\"physics\");\n"
                << "                        boolean enabled = phys.has(\"enabled\") && phys.get(\"enabled\").getAsBoolean();\n"
                << "                        boolean hasRb = phys.has(\"hasRigidBody\") && phys.get(\"hasRigidBody\").getAsBoolean();\n"
                << "                        if (enabled || hasRb) {\n"
                << "                            float mass = phys.has(\"mass\") ? phys.get(\"mass\").getAsFloat() : 0.0f;\n"
                << "                            CollisionShape shape = (mass == 0.0f)\n"
                << "                                ? CollisionShapeFactory.createMeshShape(spatial)\n"
                << "                                : CollisionShapeFactory.createDynamicMeshShape(spatial);\n"
                << "                            RigidBodyControl rbc = new RigidBodyControl(shape, mass);\n"
                << "                            spatial.addControl(rbc);\n"
                << "                            bulletAppState.getPhysicsSpace().add(rbc);\n"
                << "                        }\n"
                << "                    }\n\n"
                << "                    sceneRoot.attachChild(spatial);\n"
                << "                }\n"
                << "            }\n"
                << "            System.out.println(\"[SceneLoader] Loaded successfully into jMonkeyEngine! Total spatials: \" + sceneRoot.getQuantity());\n"
                << "        } catch (Exception e) {\n"
                << "            e.printStackTrace();\n"
                << "        }\n"
                << "        return sceneRoot;\n"
                << "    }\n"
                << "}\n";
        javaOfs.close();
        printf("[jME Export] Generated Java helper loader at %s\n", javaLoaderPath.c_str());
    }

    return true;
}

void Scene::loadScene(const std::string& filename, AssetManager* assetManager, int cubeMeshId, int sphereMeshId, int planeMeshId, GizmoSystem* gizmo, VulkanRenderer* renderer, PhysEngine* physEngine)
{
    cameraSettings = CameraSettings();
    std::string actualPath = filename;
    if (!std::filesystem::exists(actualPath))
    {
        std::vector<std::string> searchPaths = {
            "build/Release/" + filename,
            "scenes/" + filename,
            "../" + filename,
            "../../" + filename,
            "scene.json", "scene.js",
            "build/Release/scene.json", "build/Release/scene.js",
            "build/Debug/scene.json", "build/Debug/scene.js",
            "../scene.json", "../scene.js"
        };
        for (const auto& sp : searchPaths)
        {
            if (std::filesystem::exists(sp))
            {
                actualPath = sp;
                break;
            }
        }
    }

    std::ifstream in(actualPath);
    if (!in.is_open())
    {
        printf("[Scene] Could not open scene file: %s\n", actualPath.c_str());
        return;
    }

    printf("[Scene] Loading scene from: %s\n", actualPath.c_str());
    currentScenePath = actualPath;

    // Clean up old physics bodies
    if (physEngine)
    {
        for (auto& obj : sceneObjects)
        {
            if (obj.bodyData)
            {
                physEngine->removeBody(obj.bodyData);
                obj.bodyData = nullptr;
            }
        }
    }
    sceneObjects.clear();

    if (assetManager)
    {
        if (cubeMeshId < 0) cubeMeshId = assetManager->getCubeMeshId();
        if (sphereMeshId < 0) sphereMeshId = assetManager->getSphereMeshId();
        if (planeMeshId < 0) planeMeshId = assetManager->getPlaneMeshId();
    }

    std::string line;
    SceneObject current;
    std::string currentPrimitiveMesh = "";
    std::string currentTerrainParams = "";
    bool currentHasPBR = false;
    bool currentHasTextureTag = false;
    bool currentHasRigidBody = false;
    bool currentHasLight = false;
    float currentLightIntensity = 1.0f;
    glm::vec4 currentLightColor = glm::vec4(1.0f);
    bool currentMeshVisible = true;
    bool hasObj = false;

    // RigidBody temporary storage
    float rbMass = 1.0f;
    float rbFriction = 0.5f;
    float rbRestitution = 0.3f;
    float rbLinearDrag = 0.01f;
    float rbAngularDrag = 0.05f;
    int rbColliderType = 0;
    int rbMotionType = 2; // Dynamic
    bool rbUseGravity = true;
    bool rbIsTrigger = false;
    glm::vec3 rbVelocity = glm::vec3(0.0f);
    glm::vec3 rbAngularVelocity = glm::vec3(0.0f);

    auto resetCurrentState = [&]() {
        current = SceneObject();
        currentPrimitiveMesh = "";
        currentTerrainParams = "";
        currentHasPBR = false;
        currentHasTextureTag = false;
        currentHasRigidBody = false;
        currentHasLight = false;
        currentLightIntensity = 1.0f;
        currentLightColor = glm::vec4(1.0f);
        currentMeshVisible = true;
        rbMass = 1.0f;
        rbFriction = 0.5f;
        rbRestitution = 0.3f;
        rbLinearDrag = 0.01f;
        rbAngularDrag = 0.05f;
        rbColliderType = 0;
        rbMotionType = 2;
        rbUseGravity = true;
        rbIsTrigger = false;
        rbVelocity = glm::vec3(0.0f);
        rbAngularVelocity = glm::vec3(0.0f);
    };

    auto finalizeObject = [&](SceneObject& obj) {
        // 1. Procedural Terrain Mesh
        if (!currentTerrainParams.empty() && assetManager)
        {
            std::stringstream tss(currentTerrainParams);
            int gw = 35, gd = 35, seed = 42, biome = 0;
            float cs = 1.0f, hs = 5.0f;
            tss >> gw >> gd >> cs >> hs >> seed >> biome;
            int tMeshId = assetManager->createTerrainMesh(gw, gd, cs, hs, seed, biome);
            if (tMeshId >= 0)
            {
                obj.meshId = tMeshId;
                obj.type = ObjectType::PLANE;
            }
        }
        // 2. Explicit 3D Model file
        else if (!obj.modelPath.empty() && assetManager)
        {
            // Normalize slashes
            for (auto& c : obj.modelPath) if (c == '\\') c = '/';

            if (obj.modelPath.rfind("terrain:", 0) == 0)
            {
                std::stringstream tss(obj.modelPath.substr(8));
                std::string wStr, dStr, csStr, hsStr, seedStr, biomeStr;
                std::getline(tss, wStr, ':');
                std::getline(tss, dStr, ':');
                std::getline(tss, csStr, ':');
                std::getline(tss, hsStr, ':');
                std::getline(tss, seedStr, ':');
                std::getline(tss, biomeStr, ':');
                try {
                    int gw = std::stoi(wStr), gd = std::stoi(dStr), seed = std::stoi(seedStr), biome = std::stoi(biomeStr);
                    float cs = std::stof(csStr), hs = std::stof(hsStr);
                    int tMeshId = assetManager->createTerrainMesh(gw, gd, cs, hs, seed, biome);
                    obj.meshId = tMeshId;
                    obj.type = ObjectType::PLANE;
                } catch (...) {}
            }
            else
            {
                int mId = assetManager->getOrLoadModelAsset(obj.modelPath);
                if (mId >= 0)
                {
                    obj.meshId = mId;
                    if (glm::length(obj.scale) < 0.001f) obj.scale = glm::vec3(1.0f);
                    const auto& meshes = assetManager->getMeshes();
                    if (mId < static_cast<int>(meshes.size()))
                    {
                        // Default model texture if not explicitly specified
                        if (obj.texturePath.empty() && meshes[mId].defaultTextureId >= 0 && !currentHasTextureTag)
                            obj.textureId = meshes[mId].defaultTextureId;
                        
                        // ONLY fallback to mesh default PBR if file has NO saved PBR parameters!
                        if (!currentHasPBR)
                        {
                            obj.roughness = meshes[mId].defaultRoughness;
                            obj.metallic = meshes[mId].defaultMetallic;
                        }
                    }
                }
            }
        }
        // 3. Explicit Primitive Mesh
        else if (!currentPrimitiveMesh.empty())
        {
            if (currentPrimitiveMesh == "CUBE") obj.meshId = cubeMeshId;
            else if (currentPrimitiveMesh == "SPHERE") obj.meshId = sphereMeshId;
            else if (currentPrimitiveMesh == "PLANE") obj.meshId = planeMeshId;
            else if (currentPrimitiveMesh == "CYLINDER") obj.meshId = assetManager ? assetManager->getCylinderMeshId() : -1;
            else if (currentPrimitiveMesh == "CONE") obj.meshId = assetManager ? assetManager->getConeMeshId() : -1;
            else if (currentPrimitiveMesh == "NONE") obj.meshId = -1;
            else obj.meshId = cubeMeshId;
        }
        // 4. Legacy file fallback (no model, no primitive tag)
        else if (assetManager)
        {
            std::string lowerName = obj.name;
            for (auto& c : lowerName) c = static_cast<char>(tolower(c));
            if (lowerName.find("terrain") != std::string::npos || lowerName.find("landscape") != std::string::npos || lowerName.find("dunes") != std::string::npos)
            {
                int biome = (lowerName.find("dunes") != std::string::npos || lowerName.find("desert") != std::string::npos) ? 1 : 0;
                int tMeshId = assetManager->createTerrainMesh(35, 35, 1.0f, 5.0f, 42, biome);
                obj.meshId = tMeshId;
                obj.type = ObjectType::PLANE;
            }
            else
            {
                std::string matchModel = findMatchingModelAsset(obj.name);
                if (!matchModel.empty() && obj.type == ObjectType::CUBE)
                {
                    int mId = assetManager->getOrLoadModelAsset(matchModel);
                    if (mId >= 0)
                    {
                        obj.meshId = mId;
                        obj.modelPath = matchModel;
                        if (glm::length(obj.scale) < 0.001f) obj.scale = glm::vec3(1.0f);
                        const auto& meshes = assetManager->getMeshes();
                        if (mId < static_cast<int>(meshes.size()))
                        {
                            if (obj.texturePath.empty() && meshes[mId].defaultTextureId >= 0 && !currentHasTextureTag)
                                obj.textureId = meshes[mId].defaultTextureId;
                            if (!currentHasPBR)
                            {
                                obj.roughness = meshes[mId].defaultRoughness;
                                obj.metallic = meshes[mId].defaultMetallic;
                            }
                        }
                    }
                }
            }
        }

        // Fallback mesh if still unassigned
        if (obj.meshId < 0 && currentPrimitiveMesh != "NONE")
        {
            if (obj.type == ObjectType::SPHERE) obj.meshId = sphereMeshId;
            else if (obj.type == ObjectType::PLANE) obj.meshId = planeMeshId;
            else if (obj.type == ObjectType::LIGHT) obj.meshId = cubeMeshId;
            else obj.meshId = cubeMeshId;
        }

        // Texture assignment
        if (obj.texturePath == "none")
        {
            obj.textureId = -1;
            obj.texturePath = "";
        }
        else if (!obj.texturePath.empty() && assetManager)
        {
            for (auto& c : obj.texturePath) if (c == '\\') c = '/';
            int tId = assetManager->getOrLoadTextureAsset(obj.texturePath);
            if (tId >= 0) obj.textureId = tId;
        }
        else if (!currentHasTextureTag && obj.textureId < 0 && assetManager)
        {
            // Legacy backwards-compatibility guessing only if no tag was ever written
            std::string lowerName = obj.name;
            for (auto& c : lowerName) c = static_cast<char>(tolower(c));
            if (lowerName.find("terrain") != std::string::npos || lowerName.find("landscape") != std::string::npos ||
                lowerName.find("ground") != std::string::npos || (obj.type == ObjectType::PLANE && lowerName.find("plane") != std::string::npos))
            {
                int gTex = assetManager->getOrLoadTextureAsset("assets/grass.jpg");
                if (gTex >= 0)
                {
                    obj.textureId = gTex;
                    obj.texturePath = "assets/grass.jpg";
                    if (!currentHasPBR)
                    {
                        obj.usePBR = true;
                        obj.roughness = 0.85f;
                        obj.metallic = 0.05f;
                    }
                }
            }
        }

        // Setup RigidBody Component
        if (currentHasRigidBody || obj.isPhysicsEnabled)
        {
            auto rb = obj.getComponent<RigidBodyComponent>();
            if (!rb)
            {
                rb = std::make_shared<RigidBodyComponent>();
                obj.components.push_back(rb);
            }
            rb->mass = rbMass;
            rb->friction = rbFriction;
            rb->restitution = rbRestitution;
            rb->linearDrag = rbLinearDrag;
            rb->angularDrag = rbAngularDrag;
            rb->colliderType = static_cast<ColliderType>(rbColliderType);
            rb->motionType = static_cast<BodyMotionType>(rbMotionType);
            rb->useGravity = rbUseGravity;
            rb->isTrigger = rbIsTrigger;
            rb->velocity = rbVelocity;
            rb->angularVelocity = rbAngularVelocity;
            obj.velocity = rbVelocity;
        }

        // Setup Light Component
        if (currentHasLight || obj.type == ObjectType::LIGHT)
        {
            auto light = obj.getComponent<LightComponent>();
            if (!light)
            {
                light = std::make_shared<LightComponent>();
                obj.components.push_back(light);
            }
            light->color = currentLightColor;
            light->intensity = currentLightIntensity;
        }

        // Setup MeshRenderer Component
        auto mr = obj.getComponent<MeshRendererComponent>();
        if (!mr && obj.meshId >= 0)
        {
            mr = std::make_shared<MeshRendererComponent>();
            obj.components.push_back(mr);
        }
        if (mr)
        {
            mr->meshId = obj.meshId;
            mr->textureId = obj.textureId;
            mr->color = obj.color;
            mr->visible = currentMeshVisible;
        }

        obj.syncComponents();
    };

    while (std::getline(in, line))
    {
        // Strip CR from Windows CRLF
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "camera_scene:")
        {
            ss >> cameraSettings.sceneCameraTarget.x 
               >> cameraSettings.sceneCameraTarget.y 
               >> cameraSettings.sceneCameraTarget.z 
               >> cameraSettings.sceneRotationX 
               >> cameraSettings.sceneRotationY 
               >> cameraSettings.sceneCameraDistance;
            cameraSettings.hasSavedCamera = true;
        }
        else if (key == "camera_main:")
        {
            ss >> cameraSettings.mainCameraPos.x 
               >> cameraSettings.mainCameraPos.y 
               >> cameraSettings.mainCameraPos.z 
               >> cameraSettings.mainCameraTarget.x 
               >> cameraSettings.mainCameraTarget.y 
               >> cameraSettings.mainCameraTarget.z 
               >> cameraSettings.mainCameraFov 
               >> cameraSettings.mainCameraNear 
               >> cameraSettings.mainCameraFar 
               >> cameraSettings.aspectRatioMode;
            cameraSettings.hasSavedCamera = true;
        }
        else if (key == "camera_photo_frame:")
        {
            int gridInt = 0, rulersInt = 1;
            ss >> cameraSettings.photoFrameWidthMeters
               >> cameraSettings.photoFrameHeightMeters
               >> gridInt >> rulersInt;
            cameraSettings.showPhotoGrid = (gridInt == 1);
            cameraSettings.showPhotoFrameRulers = (rulersInt == 1);
        }
        else if (key == "scene_settings:")
        {
            int shadowsInt = 1;
            ss >> selectedObjectIndex >> shadowsInt
               >> playerStartPos.x >> playerStartPos.y >> playerStartPos.z;
            cameraSettings.enableShadows = (shadowsInt == 1);
        }
        else if (key == "environment:")
        {
            std::string env;
            std::getline(ss, env);
            if (!env.empty() && env[0] == ' ') env = env.substr(1);
            currentEnvironmentName = env;
        }
        else if (key == "object:")
        {
            if (hasObj)
            {
                finalizeObject(current);
                sceneObjects.push_back(current);
            }
            resetCurrentState();
            std::string name;
            std::getline(ss, name);
            if (!name.empty() && name[0] == ' ') name = name.substr(1);
            current.id = static_cast<int>(sceneObjects.size());
            current.name = name;
            hasObj = true;
        }
        else if (key == "id:")
        {
            ss >> current.id;
        }
        else if (key == "parentId:")
        {
            ss >> current.parentId;
        }
        else if (key == "type:")
        {
            int t; ss >> t;
            current.type = static_cast<ObjectType>(t);
        }
        else if (key == "model:" || key == "modelPath:")
        {
            std::string mPath;
            std::getline(ss, mPath);
            if (!mPath.empty() && mPath[0] == ' ') mPath = mPath.substr(1);
            for (auto& c : mPath) if (c == '\\') c = '/';
            current.modelPath = mPath;
        }
        else if (key == "terrainMesh:")
        {
            std::string params;
            std::getline(ss, params);
            if (!params.empty() && params[0] == ' ') params = params.substr(1);
            currentTerrainParams = params;
        }
        else if (key == "primitiveMesh:")
        {
            ss >> currentPrimitiveMesh;
        }
        else if (key == "texture:" || key == "texturePath:")
        {
            std::string tPath;
            std::getline(ss, tPath);
            if (!tPath.empty() && tPath[0] == ' ') tPath = tPath.substr(1);
            for (auto& c : tPath) if (c == '\\') c = '/';
            current.texturePath = tPath;
            currentHasTextureTag = true;
        }
        else if (key == "pbr:")
        {
            int pbrFlag = 1;
            float r = 0.5f, m = 0.0f, ao = 1.0f;
            ss >> pbrFlag >> r >> m >> ao;
            current.usePBR = (pbrFlag == 1);
            current.roughness = r;
            current.metallic = m;
            current.ambientOcclusion = ao;
            currentHasPBR = true;
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
            int p; ss >> p;
            current.isPhysicsEnabled = (p == 1);
        }
        else if (key == "hasRigidBody:")
        {
            int h; ss >> h;
            currentHasRigidBody = (h == 1);
        }
        else if (key == "mass:")
        {
            ss >> rbMass;
            currentHasRigidBody = true;
        }
        else if (key == "friction:")
        {
            ss >> rbFriction;
            currentHasRigidBody = true;
        }
        else if (key == "restitution:")
        {
            ss >> rbRestitution;
            currentHasRigidBody = true;
        }
        else if (key == "linearDrag:")
        {
            ss >> rbLinearDrag;
            currentHasRigidBody = true;
        }
        else if (key == "angularDrag:")
        {
            ss >> rbAngularDrag;
            currentHasRigidBody = true;
        }
        else if (key == "colliderType:")
        {
            ss >> rbColliderType;
            currentHasRigidBody = true;
        }
        else if (key == "motionType:")
        {
            ss >> rbMotionType;
            currentHasRigidBody = true;
        }
        else if (key == "useGravity:")
        {
            int v; ss >> v;
            rbUseGravity = (v == 1);
            currentHasRigidBody = true;
        }
        else if (key == "isTrigger:")
        {
            int v; ss >> v;
            rbIsTrigger = (v == 1);
            currentHasRigidBody = true;
        }
        else if (key == "velocity:")
        {
            ss >> rbVelocity.x >> rbVelocity.y >> rbVelocity.z;
            current.velocity = rbVelocity;
        }
        else if (key == "angularVelocity:")
        {
            ss >> rbAngularVelocity.x >> rbAngularVelocity.y >> rbAngularVelocity.z;
        }
        else if (key == "light:")
        {
            ss >> currentLightColor.r >> currentLightColor.g >> currentLightColor.b >> currentLightColor.a >> currentLightIntensity;
            currentHasLight = true;
        }
        else if (key == "meshRenderer:")
        {
            int v = 1; ss >> v;
            currentMeshVisible = (v == 1);
        }
        else if (key == "luaScriptStart:")
        {
            int hasTrailingNl = 1;
            if (!(ss >> hasTrailingNl)) hasTrailingNl = 1;
            std::string scriptContent = "";
            std::string scriptLine;
            while (std::getline(in, scriptLine))
            {
                if (!scriptLine.empty() && scriptLine.back() == '\r') scriptLine.pop_back();
                if (scriptLine == "luaScriptEnd:") break;
                scriptContent += scriptLine + "\n";
            }
            if (hasTrailingNl == 0 && !scriptContent.empty() && scriptContent.back() == '\n')
            {
                scriptContent.pop_back();
            }
            current.luaScripts.push_back(scriptContent);
        }
    }

    if (hasObj)
    {
        finalizeObject(current);
        sceneObjects.push_back(current);
    }
    in.close();

    // Rebuild Parent-Child hierarchy
    for (auto& obj : sceneObjects) obj.children.clear();
    for (size_t i = 0; i < sceneObjects.size(); ++i)
    {
        int pId = sceneObjects[i].parentId;
        if (pId >= 0 && pId < static_cast<int>(sceneObjects.size()) && pId != static_cast<int>(i))
        {
            sceneObjects[pId].children.push_back(static_cast<int>(i));
        }
    }

    // Player Cube position sync
    for (auto& obj : sceneObjects)
    {
        if (obj.name == "Player Cube" && glm::length(playerStartPos) < 0.001f)
        {
            playerStartPos = obj.position;
        }
    }

    // Apply Camera & Renderer Settings
    if (cameraSettings.hasSavedCamera)
    {
        applyCameraSettings(gizmo, renderer);
        printf("[Scene] Restored camera settings: SceneCam dist=%.1f, MainCam pos=(%.1f, %.1f, %.1f)\n",
               cameraSettings.sceneCameraDistance,
               cameraSettings.mainCameraPos.x, cameraSettings.mainCameraPos.y, cameraSettings.mainCameraPos.z);
    }
    else if (gizmo)
    {
        gizmo->frameScenicPhotoView(*this, 0);
    }

    // Bounds check selected index
    if (selectedObjectIndex < -1 || selectedObjectIndex >= static_cast<int>(sceneObjects.size()))
    {
        selectedObjectIndex = sceneObjects.empty() ? -1 : 0;
    }

    // Deduce environment name if not specified in file
    if (currentEnvironmentName.empty() || currentEnvironmentName == "Default")
    {
        for (const auto& obj : sceneObjects)
        {
            std::string lowerObj = obj.name;
            for (auto& c : lowerObj) c = static_cast<char>(tolower(c));
            if (lowerObj.find("warehouse") != std::string::npos) { currentEnvironmentName = "Industrial Warehouse"; break; }
            if (lowerObj.find("forest") != std::string::npos) { currentEnvironmentName = "Forest Clearing"; break; }
            if (lowerObj.find("tactical") != std::string::npos || lowerObj.find("outpost") != std::string::npos || lowerObj.find("target") != std::string::npos) { currentEnvironmentName = "Tactical Outpost"; break; }
            if (lowerObj.find("desert") != std::string::npos || lowerObj.find("dune") != std::string::npos) { currentEnvironmentName = "Desert Dunes"; break; }
            if (lowerObj.find("city") != std::string::npos || lowerObj.find("cyber") != std::string::npos) { currentEnvironmentName = "Cyberpunk City"; break; }
            if (lowerObj.find("medieval") != std::string::npos || lowerObj.find("citadel") != std::string::npos) { currentEnvironmentName = "Medieval Citadel"; break; }
            if (lowerObj.find("island") != std::string::npos) { currentEnvironmentName = "Floating Islands"; break; }
            if (lowerObj.find("maze") != std::string::npos) { currentEnvironmentName = "3D Maze"; break; }
        }
    }

    saveHistory();
    printf("[Scene] Loaded %zu objects from %s successfully!\n", sceneObjects.size(), actualPath.c_str());
}

void Scene::loadScene(const std::string& filename, int cubeMeshId, int sphereMeshId, int planeMeshId)
{
    loadScene(filename, nullptr, cubeMeshId, sphereMeshId, planeMeshId, nullptr, nullptr, nullptr);
}

void Scene::initializeDefaultScene(int cubeMeshId, int sphereMeshId, int planeMeshId, int groundTexId, PhysEngine* physEngine)
{
    sceneObjects.clear();
    
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
    player.meshId = cubeMeshId;

    auto playerRb = std::make_shared<RigidBodyComponent>();
    playerRb->colliderType = ColliderType::BOX;
    playerRb->motionType = BodyMotionType::DYNAMIC;
    playerRb->mass = 1.0f;
    player.components.push_back(playerRb);

    std::string playerLuaScript = R"(local PlayerWASD = {}

function PlayerWASD:onStart(obj)
    self.speed = 4.0
    print("[Lua] PlayerWASD started on: " .. obj.name)
end

function PlayerWASD:onUpdate(obj, dt)
    local moveX = 0.0
    local moveZ = 0.0

    if Input.isKeyPressed("W") or Input.isKeyPressed("UP") then moveZ = moveZ - 1.0 end
    if Input.isKeyPressed("S") or Input.isKeyPressed("DOWN") then moveZ = moveZ + 1.0 end
    if Input.isKeyPressed("A") or Input.isKeyPressed("LEFT") then moveX = moveX - 1.0 end
    if Input.isKeyPressed("D") or Input.isKeyPressed("RIGHT") then moveX = moveX + 1.0 end

    if moveX ~= 0.0 and moveZ ~= 0.0 then
        moveX = moveX * 0.7071
        moveZ = moveZ * 0.7071
    end

    obj.position.x = obj.position.x + moveX * self.speed * dt
    obj.position.z = obj.position.z + moveZ * self.speed * dt

    if moveX ~= 0.0 or moveZ ~= 0.0 then
        local angle = math.deg(math.atan(moveX, -moveZ))
        obj.rotation.y = angle
    end

    if Input.isKeyPressed("SPACE") and obj.position.y <= 0.05 then
        obj.velocity.y = 5.0
    end
end

return PlayerWASD
)";
    player.luaScripts.push_back(playerLuaScript);

    std::string respawnLuaScript = R"(local RespawnOnFall = {}

function RespawnOnFall:onStart(obj)
    self.fallThreshold = -10.0
    self.spawnPos = { x = obj.position.x, y = obj.position.y + 2.0, z = obj.position.z }
    print("[Lua] RespawnOnFall initialized for: " .. obj.name)
end

function RespawnOnFall:onUpdate(obj, dt)
    if obj.position.y < self.fallThreshold then
        print("[Lua] " .. obj.name .. " fell out of bounds! Respawning...")
        obj.position.x = self.spawnPos.x
        obj.position.y = self.spawnPos.y
        obj.position.z = self.spawnPos.z

        obj.velocity.x = 0.0
        obj.velocity.y = 0.0
        obj.velocity.z = 0.0
    end
end

return RespawnOnFall
)";
    player.luaScripts.push_back(respawnLuaScript);
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
    target.isPhysicsEnabled = true;
    target.meshId = sphereMeshId;

    auto targetRb = std::make_shared<RigidBodyComponent>();
    targetRb->colliderType = ColliderType::SPHERE;
    targetRb->motionType = BodyMotionType::DYNAMIC;
    targetRb->mass = 0.5f;
    target.components.push_back(targetRb);

    std::string goldLuaScript = R"(local GoldCollectible = {}

function GoldCollectible:onStart(obj)
    self.pickupRadius = 0.6
    print("[Lua] GoldCollectible initialized for: " .. obj.name)
end

function GoldCollectible:onUpdate(obj, dt)
    obj.rotation.y = obj.rotation.y + 90.0 * dt
    local playerPos = Game.getPlayerPosition()

    local dx = obj.position.x - playerPos.x
    local dy = obj.position.y - playerPos.y
    local dz = obj.position.z - playerPos.z
    local dist = math.sqrt(dx * dx + dy * dy + dz * dz)

    if dist < self.pickupRadius then
        print("[Lua] Gold Collected! +1 Score")
        Game.addScore(1)
        obj.position.x = math.random(-30, 30) / 10.0
        obj.position.y = -1.2
        obj.position.z = math.random(-30, 30) / 10.0
    end
end

return GoldCollectible
)";
    target.luaScripts.push_back(goldLuaScript);
    sceneObjects.push_back(target);

    // 3. Ground Plane
    SceneObject ground;
    ground.id = 2;
    ground.name = "Ground Obstacle";
    ground.type = ObjectType::PLANE;
    ground.position = glm::vec3(0.0f, -1.5f, 0.0f);
    ground.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    ground.scale = glm::vec3(5.0f, 0.1f, 5.0f);
    ground.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ground.roughness = 0.85f;
    ground.metallic = 0.1f;
    ground.usePBR = true;
    ground.isPhysicsEnabled = true;
    ground.meshId = planeMeshId;
    ground.textureId = groundTexId;
    ground.texturePath = "assets/grass.jpg";

    auto groundRb = std::make_shared<RigidBodyComponent>();
    groundRb->colliderType = ColliderType::PLANE;
    groundRb->motionType = BodyMotionType::STATIC;
    groundRb->mass = 0.0f;
    ground.components.push_back(groundRb);
    sceneObjects.push_back(ground);

    // 4. Directional Light
    SceneObject sun;
    sun.id = 3;
    sun.name = "Directional Light";
    sun.type = ObjectType::LIGHT;
    sun.position = glm::vec3(2.0f, 3.0f, 1.0f);
    sun.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
    sun.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    sun.color = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);
    sun.isPhysicsEnabled = false;
    sun.meshId = cubeMeshId;
    sceneObjects.push_back(sun);

    selectedObjectIndex = 0;
    if (physEngine)
    {
        initPhysicsBodies(physEngine);
    }
    saveHistory();
}

void Scene::initPhysicsBodies(PhysEngine* physEngine)
{
    if (!physEngine) return;
    for (auto& obj : sceneObjects)
    {
        if (obj.bodyData)
        {
            physEngine->removeBody(obj.bodyData);
            obj.bodyData = nullptr;
        }

        if (obj.isPhysicsEnabled || obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS))
        {
            auto rb = obj.getComponent<RigidBodyComponent>();
            if (!rb)
            {
                rb = std::make_shared<RigidBodyComponent>();
                if (obj.type == ObjectType::SPHERE) rb->colliderType = ColliderType::SPHERE;
                else if (obj.type == ObjectType::PLANE) {
                    rb->colliderType = ColliderType::PLANE;
                    rb->motionType = BodyMotionType::STATIC;
                }
                obj.components.push_back(rb);
            }

            obj.isPhysicsEnabled = true;

            ColliderType ct = rb->colliderType;
            BodyMotionType mt = rb->motionType;
            float mass = rb->mass;
            float friction = rb->friction;
            float restitution = rb->restitution;

            glm::quat rotQuat = glm::quat(glm::radians(obj.rotation));
            obj.bodyData = physEngine->createBody(ct, obj.position, rotQuat, obj.scale, mt, mass, friction, restitution);
        }
    }
}

void Scene::syncPhysicsToTransform(PhysEngine* physEngine)
{
    if (!physEngine) return;
    for (auto& obj : sceneObjects)
    {
        if (!obj.bodyData) continue;

        auto rb = obj.getComponent<RigidBodyComponent>();
        if (!rb) continue;
        if (rb->motionType == BodyMotionType::STATIC) continue;

        obj.position = physEngine->getBodyPosition(obj.bodyData);
        glm::quat q = physEngine->getBodyRotation(obj.bodyData);
        obj.rotation = glm::degrees(glm::eulerAngles(q));
        obj.velocity = physEngine->getLinearVelocity(obj.bodyData);

        if (rb->linearDrag > 0.0f)
        {
            glm::vec3 vel = obj.velocity;
            vel *= (1.0f - rb->linearDrag);
            physEngine->setLinearVelocity(obj.bodyData, vel);
        }

        if (rb->angularDrag > 0.0f)
        {
            glm::vec3 avel = physEngine->getAngularVelocity(obj.bodyData);
            avel *= (1.0f - rb->angularDrag);
            physEngine->setAngularVelocity(obj.bodyData, avel);
        }
    }
}
