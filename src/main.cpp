#include "VulkanApp.h"
#include "scene/Scene.h"
#include "assets/ModelLoader.h"
#include "animation/AnimationLoader.h"
#include "editor/SceneGenerator.h"
#include "assets/AssetManager.h"
#include <iostream>
#include <exception>
#include <cstdlib>
#include <cmath>
#include <filesystem>

static bool runSceneSaveLoadTest()
{
    std::cout << "[Test] Starting Scene Save & Load Verification Test...\n";

    Scene scene1;
    scene1.setSelectedObjectIndex(1);
    scene1.setPlayerStartPos(glm::vec3(5.0f, 0.0f, 5.0f));

    CameraSettings& cam1 = scene1.getCameraSettings();
    cam1.sceneRotationX = 35.0f;
    cam1.sceneRotationY = 75.0f;
    cam1.sceneCameraDistance = 18.5f;
    cam1.sceneCameraTarget = glm::vec3(1.0f, 2.0f, 3.0f);
    cam1.mainCameraPos = glm::vec3(4.0f, 5.0f, 6.0f);
    cam1.mainCameraTarget = glm::vec3(0.0f, 1.0f, 0.0f);
    cam1.mainCameraFov = 60.0f;
    cam1.mainCameraNear = 0.2f;
    cam1.mainCameraFar = 250.0f;
    cam1.aspectRatioMode = 1;
    cam1.photoFrameWidthMeters = 3.2f;
    cam1.photoFrameHeightMeters = 2.4f;
    cam1.showPhotoGrid = true;
    cam1.showPhotoFrameRulers = true;
    cam1.enableShadows = false;
    cam1.hasSavedCamera = true;

    // Object 0: Primitive Cube with Physics & PBR
    SceneObject obj0;
    obj0.id = 0;
    obj0.name = "Test Box Entity";
    obj0.type = ObjectType::CUBE;
    obj0.position = glm::vec3(1.2f, 2.3f, 3.4f);
    obj0.rotation = glm::vec3(15.0f, 30.0f, 45.0f);
    obj0.scale = glm::vec3(0.8f, 1.2f, 2.0f);
    obj0.color = glm::vec4(1.0f, 0.2f, 0.3f, 1.0f);
    obj0.usePBR = true;
    obj0.roughness = 0.35f;
    obj0.metallic = 0.85f;
    obj0.ambientOcclusion = 0.9f;
    obj0.isPhysicsEnabled = true;
    obj0.velocity = glm::vec3(1.0f, 2.0f, 3.0f);
    auto rb0 = std::make_shared<RigidBodyComponent>();
    rb0->mass = 4.5f;
    rb0->friction = 0.65f;
    rb0->restitution = 0.75f;
    rb0->linearDrag = 0.03f;
    rb0->angularDrag = 0.09f;
    rb0->colliderType = ColliderType::BOX;
    rb0->motionType = BodyMotionType::DYNAMIC;
    rb0->useGravity = true;
    rb0->isTrigger = false;
    rb0->velocity = obj0.velocity;
    rb0->angularVelocity = glm::vec3(0.5f, 0.6f, 0.7f);
    obj0.components.push_back(rb0);
    obj0.syncComponents();
    scene1.getObjects().push_back(obj0);

    // Object 1: Model child with Lua Script
    SceneObject obj1;
    obj1.id = 1;
    obj1.parentId = 0;
    obj1.name = "Pine Tree Model Child";
    obj1.type = ObjectType::CUBE;
    obj1.position = glm::vec3(-4.0f, 0.5f, 7.5f);
    obj1.rotation = glm::vec3(0.0f, 120.0f, 0.0f);
    obj1.scale = glm::vec3(1.0f, 1.5f, 1.0f);
    obj1.color = glm::vec4(1.0f);
    obj1.modelPath = "assets/models/tree_pine.obj";
    obj1.usePBR = true;
    obj1.roughness = 0.6f;
    obj1.metallic = 0.05f;
    obj1.ambientOcclusion = 1.0f;
    std::string testScript = "local Test = {}\nfunction Test:onUpdate(obj, dt)\n    print(\"Hello\")\nend\nreturn Test\n";
    obj1.luaScripts.push_back(testScript);
    obj1.syncComponents();
    scene1.getObjects().push_back(obj1);

    // Object 2: Light entity
    SceneObject obj2;
    obj2.id = 2;
    obj2.name = "Sun Light";
    obj2.type = ObjectType::LIGHT;
    obj2.position = glm::vec3(0.0f, 10.0f, 5.0f);
    obj2.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
    obj2.scale = glm::vec3(0.5f);
    obj2.color = glm::vec4(1.0f, 0.95f, 0.8f, 1.0f);
    auto light2 = std::make_shared<LightComponent>();
    light2->color = obj2.color;
    light2->intensity = 3.0f;
    obj2.components.push_back(light2);
    obj2.syncComponents();
    scene1.getObjects().push_back(obj2);

    // Save scene1
    std::filesystem::create_directories("build");
    std::string testFile = "build/test_roundtrip.json";
    scene1.saveScene(testFile, nullptr, nullptr, nullptr);

    // Load into scene2
    Scene scene2;
    scene2.loadScene(testFile, nullptr, -1, -1, -1, nullptr, nullptr, nullptr);

    auto floatEq = [](float a, float b) { return std::abs(a - b) < 0.001f; };
    auto vec3Eq = [&](const glm::vec3& a, const glm::vec3& b) {
        return floatEq(a.x, b.x) && floatEq(a.y, b.y) && floatEq(a.z, b.z);
    };

    if (scene2.getObjects().size() != scene1.getObjects().size())
    {
        std::cerr << "[TEST FAIL] Object count mismatch: " << scene2.getObjects().size() << " vs " << scene1.getObjects().size() << "\n";
        return false;
    }

    const CameraSettings& cam2 = scene2.getCameraSettings();
    if (!vec3Eq(cam1.sceneCameraTarget, cam2.sceneCameraTarget) ||
        !floatEq(cam1.sceneRotationX, cam2.sceneRotationX) ||
        !floatEq(cam1.sceneRotationY, cam2.sceneRotationY) ||
        !floatEq(cam1.sceneCameraDistance, cam2.sceneCameraDistance) ||
        !vec3Eq(cam1.mainCameraPos, cam2.mainCameraPos) ||
        !vec3Eq(cam1.mainCameraTarget, cam2.mainCameraTarget) ||
        !floatEq(cam1.mainCameraFov, cam2.mainCameraFov) ||
        cam1.aspectRatioMode != cam2.aspectRatioMode ||
        !floatEq(cam1.photoFrameWidthMeters, cam2.photoFrameWidthMeters) ||
        !floatEq(cam1.photoFrameHeightMeters, cam2.photoFrameHeightMeters) ||
        cam1.showPhotoGrid != cam2.showPhotoGrid ||
        cam1.showPhotoFrameRulers != cam2.showPhotoFrameRulers ||
        cam1.enableShadows != cam2.enableShadows)
    {
        std::cerr << "[TEST FAIL] Camera settings mismatch!\n";
        return false;
    }

    if (scene1.getSelectedObjectIndex() != scene2.getSelectedObjectIndex())
    {
        std::cerr << "[TEST FAIL] Selected object index mismatch!\n";
        return false;
    }
    if (!vec3Eq(scene1.getPlayerStartPos(), scene2.getPlayerStartPos()))
    {
        std::cerr << "[TEST FAIL] PlayerStartPos mismatch!\n";
        return false;
    }

    for (size_t i = 0; i < scene1.getObjects().size(); ++i)
    {
        const auto& o1 = scene1.getObjects()[i];
        const auto& o2 = scene2.getObjects()[i];

        if (o1.name != o2.name) { std::cerr << "[TEST FAIL] Name mismatch: " << o1.name << " vs " << o2.name << "\n"; return false; }
        if (o1.id != o2.id) { std::cerr << "[TEST FAIL] ID mismatch!\n"; return false; }
        if (o1.parentId != o2.parentId) { std::cerr << "[TEST FAIL] parentId mismatch!\n"; return false; }
        if (o1.type != o2.type) { std::cerr << "[TEST FAIL] type mismatch!\n"; return false; }
        if (!vec3Eq(o1.position, o2.position)) { std::cerr << "[TEST FAIL] position mismatch on " << o1.name << "\n"; return false; }
        if (!vec3Eq(o1.rotation, o2.rotation)) { std::cerr << "[TEST FAIL] rotation mismatch on " << o1.name << "\n"; return false; }
        if (!vec3Eq(o1.scale, o2.scale)) { std::cerr << "[TEST FAIL] scale mismatch on " << o1.name << "\n"; return false; }
        if (!floatEq(o1.color.r, o2.color.r) || !floatEq(o1.color.g, o2.color.g) || !floatEq(o1.color.b, o2.color.b) || !floatEq(o1.color.a, o2.color.a))
        { std::cerr << "[TEST FAIL] color mismatch on " << o1.name << "\n"; return false; }

        if (o1.usePBR != o2.usePBR || !floatEq(o1.roughness, o2.roughness) || !floatEq(o1.metallic, o2.metallic) || !floatEq(o1.ambientOcclusion, o2.ambientOcclusion))
        { std::cerr << "[TEST FAIL] PBR mismatch on " << o1.name << " (roughness: " << o1.roughness << " vs " << o2.roughness << ", metallic: " << o1.metallic << " vs " << o2.metallic << ")\n"; return false; }

        if (o1.modelPath != o2.modelPath) { std::cerr << "[TEST FAIL] modelPath mismatch on " << o1.name << ": " << o1.modelPath << " vs " << o2.modelPath << "\n"; return false; }

        if (o1.isPhysicsEnabled != o2.isPhysicsEnabled) { std::cerr << "[TEST FAIL] isPhysicsEnabled mismatch on " << o1.name << "\n"; return false; }

        auto rb1 = o1.getComponent<RigidBodyComponent>();
        auto rb2 = o2.getComponent<RigidBodyComponent>();
        if ((rb1 != nullptr) != (rb2 != nullptr)) { std::cerr << "[TEST FAIL] RigidBody existence mismatch on " << o1.name << "\n"; return false; }
        if (rb1 && rb2)
        {
            if (!floatEq(rb1->mass, rb2->mass) || !floatEq(rb1->friction, rb2->friction) || !floatEq(rb1->restitution, rb2->restitution) ||
                !floatEq(rb1->linearDrag, rb2->linearDrag) || !floatEq(rb1->angularDrag, rb2->angularDrag) ||
                rb1->colliderType != rb2->colliderType || rb1->motionType != rb2->motionType ||
                rb1->useGravity != rb2->useGravity || rb1->isTrigger != rb2->isTrigger ||
                !vec3Eq(rb1->velocity, rb2->velocity) || !vec3Eq(rb1->angularVelocity, rb2->angularVelocity))
            {
                std::cerr << "[TEST FAIL] RigidBody properties mismatch on " << o1.name << "\n";
                return false;
            }
        }

        if (o1.luaScripts.size() != o2.luaScripts.size()) { std::cerr << "[TEST FAIL] Lua scripts count mismatch on " << o1.name << "\n"; return false; }
        for (size_t s = 0; s < o1.luaScripts.size(); ++s)
        {
            if (o1.luaScripts[s] != o2.luaScripts[s]) { std::cerr << "[TEST FAIL] Lua script content mismatch on " << o1.name << "\n"; return false; }
        }
    }

    if (scene2.getObjects()[0].children.empty() || scene2.getObjects()[0].children[0] != 1)
    {
        std::cerr << "[TEST FAIL] Parent-child children vector link failed!\n";
        return false;
    }

    std::cout << "[Test PASS] Synthesized round-trip test passed with 100% precision!\n";

    // Test real scene.json file round-trip
    if (std::filesystem::exists("scene.json"))
    {
        std::cout << "[Test] Verifying real scene.json file round-trip...\n";
        Scene realScene1;
        realScene1.loadScene("scene.json", nullptr, -1, -1, -1, nullptr, nullptr, nullptr);
        std::string realOutFile = "build/test_real_scene_roundtrip.json";
        realScene1.saveScene(realOutFile, nullptr, nullptr, nullptr);

        Scene realScene2;
        realScene2.loadScene(realOutFile, nullptr, -1, -1, -1, nullptr, nullptr, nullptr);

        if (realScene1.getObjects().size() != realScene2.getObjects().size())
        {
            std::cerr << "[TEST FAIL] Real scene object count mismatch: " << realScene2.getObjects().size() << " vs " << realScene1.getObjects().size() << "\n";
            return false;
        }

        for (size_t i = 0; i < realScene1.getObjects().size(); ++i)
        {
            const auto& o1 = realScene1.getObjects()[i];
            const auto& o2 = realScene2.getObjects()[i];
            if (o1.name != o2.name) { std::cerr << "[TEST FAIL] Name mismatch: " << o1.name << " vs " << o2.name << "\n"; return false; }
            if (!vec3Eq(o1.position, o2.position)) { std::cerr << "[TEST FAIL] position mismatch on " << o1.name << "\n"; return false; }
            if (!vec3Eq(o1.rotation, o2.rotation)) { std::cerr << "[TEST FAIL] rotation mismatch on " << o1.name << "\n"; return false; }
            if (!vec3Eq(o1.scale, o2.scale)) { std::cerr << "[TEST FAIL] scale mismatch on " << o1.name << "\n"; return false; }
            if (o1.usePBR != o2.usePBR || !floatEq(o1.roughness, o2.roughness) || !floatEq(o1.metallic, o2.metallic))
            {
                std::cerr << "[TEST FAIL] PBR mismatch on " << o1.name << "\n";
                return false;
            }
        }
        std::cout << "[Test PASS] Real scene.json (" << realScene1.getObjects().size() << " objects) round-trip passed with 100% precision!\n";

        // Test Model Loading of ShootingTarget.fbx
        std::cout << "[Test] Verifying ShootingTarget.fbx model loading and 1:1 scale normalization...\n";
        Mesh targetMesh;
        ModelLoader::loadModel("assets/models/ShootingTarget.fbx", targetMesh, nullptr);
        if (targetMesh.vertices.empty() || targetMesh.indices.empty())
        {
            std::cerr << "[TEST FAIL] ShootingTarget.fbx failed to load vertices/indices!\n";
            return false;
        }
        glm::vec3 minV(1e9f), maxV(-1e9f);
        for (const auto& v : targetMesh.vertices)
        {
            minV = glm::min(minV, v.pos);
            maxV = glm::max(maxV, v.pos);
        }
        float width = maxV.x - minV.x;
        float height = maxV.y - minV.y;
        std::cout << "[Test] ShootingTarget dimensions: width=" << width << "m, height=" << height << "m, minY=" << minV.y << "m, maxY=" << maxV.y << "m\n";
        if (std::abs(width - 0.7984f) > 0.05f || std::abs(height - 1.755f) > 0.05f || std::abs(minV.y) > 0.01f)
        {
            std::cerr << "[TEST FAIL] ShootingTarget scale is not 1:1 metric scale grounded at Y=0!\n";
            return false;
        }
        std::cout << "[Test PASS] ShootingTarget.fbx loads perfectly with 1:1 metric scale and grounded at Y=0!\n";

        // Test Model Loading of ShootingTarget.obj
        std::cout << "[Test] Verifying ShootingTarget.obj model loading and 1:1 scale normalization...\n";
        Mesh targetObjMesh;
        ModelLoader::loadModel("assets/models/ShootingTarget.obj", targetObjMesh, nullptr);
        if (targetObjMesh.vertices.empty() || targetObjMesh.indices.empty())
        {
            std::cerr << "[TEST FAIL] ShootingTarget.obj failed to load vertices/indices!\n";
            return false;
        }
        std::cout << "[Test PASS] ShootingTarget.obj loads successfully (" << targetObjMesh.vertices.size() << " vertices, " << targetObjMesh.indices.size() << " indices)!\n";

        // Test Model Loading of IPSC_Metric_Target.obj
        std::cout << "[Test] Verifying IPSC_Metric_Target.obj loading...\n";
        Mesh ipscMesh;
        ModelLoader::loadModel("assets/models/IPSC_Metric_Target.obj", ipscMesh, nullptr);
        if (ipscMesh.vertices.empty() || ipscMesh.indices.empty())
        {
            std::cerr << "[TEST FAIL] IPSC_Metric_Target.obj failed to load vertices/indices!\n";
            return false;
        }
        std::cout << "[Test PASS] IPSC_Metric_Target.obj loads successfully (" << ipscMesh.vertices.size() << " vertices, " << ipscMesh.indices.size() << " indices)!\n";

        // Test Model Loading of warehouse.glb
        std::cout << "[Test] Verifying warehouse.glb loading...\n";
        Mesh warehouseMesh;
        ModelLoader::loadModel("assets/models/warehouse.glb", warehouseMesh, nullptr);
        if (warehouseMesh.vertices.empty() || warehouseMesh.indices.empty())
        {
            std::cerr << "[TEST FAIL] warehouse.glb failed to load vertices/indices!\n";
            return false;
        }
        glm::vec3 wMin(1e9f), wMax(-1e9f);
        for (const auto& v : warehouseMesh.vertices)
        {
            wMin = glm::min(wMin, v.pos);
            wMax = glm::max(wMax, v.pos);
        }
        std::cout << "[Test PASS] warehouse.glb loaded successfully (" << warehouseMesh.vertices.size() << " vertices, " << warehouseMesh.indices.size() << " indices, " << warehouseMesh.submeshes.size() << " submeshes)!\n";
        std::cout << "[Test] warehouse.glb bounding box: Min=(" << wMin.x << ", " << wMin.y << ", " << wMin.z << "), Max=(" << wMax.x << ", " << wMax.y << ", " << wMax.z << ")\n";
        std::cout << "[Test] warehouse.glb dimensions: Width=" << (wMax.x - wMin.x) << "m, Height=" << (wMax.y - wMin.y) << "m, Depth=" << (wMax.z - wMin.z) << "m\n";
        for (size_t si = 0; si < warehouseMesh.submeshes.size(); ++si) {
            const auto& s = warehouseMesh.submeshes[si];
            std::cout << "  SubMesh " << si << ": indexCount=" << s.indexCount << ", texId=" << s.textureId << ", roughness=" << s.roughness << ", metallic=" << s.metallic << ", twoSided=" << s.twoSided << "\n";
        }

        // Test Industrial Warehouse 3D Scene Generation
        std::cout << "[Test] Verifying Industrial Warehouse Scene Generation...\n";
        AssetManager assetMgr;
        Scene warehouseScene;
        SceneGenerator sceneGen;
        sceneGen.setPreset(7);
        sceneGen.generate3DScene(&warehouseScene, &assetMgr, nullptr);

        if (warehouseScene.getObjects().empty())
        {
            std::cerr << "[TEST FAIL] Warehouse scene generation produced 0 objects!\n";
            return false;
        }

        bool foundWarehouseBuilding = false;
        for (const auto& obj : warehouseScene.getObjects())
        {
            if (obj.name.find("Warehouse") != std::string::npos)
            {
                foundWarehouseBuilding = true;
                break;
            }
        }
        if (!foundWarehouseBuilding)
        {
            std::cerr << "[TEST FAIL] Warehouse scene does not contain a warehouse building!\n";
            return false;
        }

        std::cout << "[Test PASS] Industrial Warehouse scene generated successfully with " << warehouseScene.getObjects().size() << " objects!\n";
        std::cout << "[Test] Environment Name: " << warehouseScene.getCurrentEnvironmentName() << ", File: " << warehouseScene.getCurrentScenePath() << "\n";

        for (const auto& obj : warehouseScene.getObjects()) {
            if (obj.name.find("Main_Warehouse_Facility") != std::string::npos && obj.meshId >= 0) {
                const auto* m = assetMgr.getMesh(obj.meshId);
                if (m) {
                    std::cout << "[Test] Main_Warehouse_Facility mesh loaded with " << m->submeshes.size() << " submeshes:\n";
                    for (size_t si = 0; si < m->submeshes.size(); ++si) {
                        const auto& sm = m->submeshes[si];
                        std::cout << "    Sub " << si << ": tex=" << sm.textureId << " ('" << assetMgr.getTexturePath(sm.textureId) << "'), rough=" << sm.roughness << ", metal=" << sm.metallic << ", twoSided=" << sm.twoSided << "\n";
                    }
                }
            }
        }

        // Save generated warehouse scene to scene_industrial_warehouse.json
        std::string whFile = "scene_industrial_warehouse.json";
        warehouseScene.saveScene(whFile, nullptr, nullptr, nullptr);
        std::cout << "[Test] Saved warehouse scene to " << whFile << "\n";

        // Verify roundtrip load of warehouse scene
        Scene loadedWarehouseScene;
        loadedWarehouseScene.loadScene(whFile, &assetMgr, -1, -1, -1, nullptr, nullptr, nullptr);
        if (loadedWarehouseScene.getObjects().size() != warehouseScene.getObjects().size())
        {
            std::cerr << "[TEST FAIL] Loaded warehouse scene object count mismatch: " << loadedWarehouseScene.getObjects().size() << " vs " << warehouseScene.getObjects().size() << "\n";
            return false;
        }

        bool verifiedWarehouseTexture = false;
        for (const auto& obj : loadedWarehouseScene.getObjects())
        {
            if (obj.name.find("Main_Warehouse_Facility") != std::string::npos)
            {
                if (obj.texturePath.empty() || obj.texturePath == "none")
                {
                    std::cerr << "[TEST FAIL] Main_Warehouse_Facility texturePath is missing or none!\n";
                    return false;
                }
                std::cout << "[Test PASS] Main_Warehouse_Facility texturePath verified: " << obj.texturePath << " (scale: " << obj.scale.x << ", " << obj.scale.y << ", " << obj.scale.z << ")\n";
                verifiedWarehouseTexture = true;
                break;
            }
        }
        if (!verifiedWarehouseTexture)
        {
            std::cerr << "[TEST FAIL] Main_Warehouse_Facility not found in loaded scene!\n";
            return false;
        }
        std::cout << "[Test PASS] Warehouse texture persistence verified successfully!\n";

        // Verify warehouse items presence
        bool foundForklift = false;
        bool foundShelving = false;
        bool foundTarget = false;
        for (const auto& obj : loadedWarehouseScene.getObjects())
        {
            if (obj.name.find("Forklift") != std::string::npos) foundForklift = true;
            if (obj.name.find("Shelving_Aisle") != std::string::npos) foundShelving = true;
            if (obj.name.find("Target") != std::string::npos) foundTarget = true;
        }
        if (!foundForklift || !foundShelving || !foundTarget)
        {
            std::cerr << "[TEST FAIL] Missing expected warehouse items (Forklift: " << foundForklift << ", Shelving: " << foundShelving << ", Target: " << foundTarget << ")!\n";
            return false;
        }
        std::cout << "[Test PASS] Warehouse logistics items (Forklift, Shelving, Shooting Targets) verified successfully!\n";

        // Verify Forest Clearing Preset (Preset 1) has shooting range targets and NO helmet or human
        std::cout << "[Test] Verifying Forest Clearing Scene Generation (Shooting range targets, no helmet/human)...\n";
        Scene forestScene;
        sceneGen.setPreset(1);
        sceneGen.generate3DScene(&forestScene, &assetMgr, nullptr);

        bool foundCesiumOrOperative = false;
        bool foundHelmet = false;
        bool foundForestShootingTarget = false;
        bool foundForestIPSC = false;
        bool foundForestPopper = false;
        bool foundForestPlate = false;
        bool foundForestBarricade = false;

        for (const auto& obj : forestScene.getObjects())
        {
            if (obj.name.find("Patrol_Operative") != std::string::npos || obj.modelPath.find("CesiumMan") != std::string::npos) foundCesiumOrOperative = true;
            if (obj.name.find("Relic_Helmet") != std::string::npos || obj.modelPath.find("DamagedHelmet") != std::string::npos) foundHelmet = true;
            if (obj.name.find("Forest_ShootingTarget") != std::string::npos) foundForestShootingTarget = true;
            if (obj.name.find("Forest_IPSC") != std::string::npos) foundForestIPSC = true;
            if (obj.name.find("Pepper_Popper") != std::string::npos) foundForestPopper = true;
            if (obj.name.find("Plate_Target") != std::string::npos) foundForestPlate = true;
            if (obj.name.find("Shooting_Barricade") != std::string::npos) foundForestBarricade = true;
        }

        if (foundCesiumOrOperative || foundHelmet)
        {
            std::cerr << "[TEST FAIL] Forest scene still contains helmet or human/operative models!\n";
            return false;
        }

        if (!foundForestShootingTarget || !foundForestIPSC || !foundForestPopper || !foundForestPlate || !foundForestBarricade)
        {
            std::cerr << "[TEST FAIL] Forest scene missing expected shooting range targets!\n";
            return false;
        }
        std::cout << "[Test PASS] Forest Clearing scene verified: replaced helmet & human with full tactical shooting range setup (100% PASS)!\n";
    }

    return true;
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::string(argv[1]) == "--test-scene")
    {
        return runSceneSaveLoadTest() ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-fbx")
    {
        std::string path = (argc > 2) ? argv[2] : "C:/Users/nguye/Downloads/Basic Shooter Pack/tm_classy_black_guy.fbx";
        std::cout << "[Test FBX] Loading: " << path << "\n";
        Mesh mesh;
        AssetManager assetMgr;
        try {
            ModelLoader::loadModel(path, mesh, &assetMgr);
            std::cout << "[Test FBX] Vertices: " << mesh.vertices.size() << "\n";
            std::cout << "[Test FBX] Indices: " << mesh.indices.size() << "\n";
            std::cout << "[Test FBX] Submeshes: " << mesh.submeshes.size() << "\n";
            std::cout << "[Test FBX] IsSkinned: " << mesh.isSkinned << "\n";
            std::cout << "[Test FBX] BoneCount: " << (mesh.skeleton ? mesh.skeleton->getBoneCount() : 0) << "\n";
            std::cout << "[Test FBX] DefaultTexId: " << mesh.defaultTextureId << "\n";
            for (size_t si = 0; si < mesh.submeshes.size(); ++si) {
                int tid = mesh.submeshes[si].textureId;
                std::string tpath = assetMgr.getTexturePath(tid);
                std::cout << "  Submesh " << si << ": tex=" << tid
                          << " ('" << tpath << "')"
                          << ", offset=" << mesh.submeshes[si].indexOffset
                          << ", count=" << mesh.submeshes[si].indexCount << "\n";
            }
            glm::vec3 minB(1e9f), maxB(-1e9f);
            for (const auto& v : mesh.vertices) {
                minB = glm::min(minB, v.pos);
                maxB = glm::max(maxB, v.pos);
            }
            std::cout << "[Test FBX] Loaded MinB: (" << minB.x << ", " << minB.y << ", " << minB.z << ")\n";
            std::cout << "[Test FBX] Loaded MaxB: (" << maxB.x << ", " << maxB.y << ", " << maxB.z << ")\n";
            std::cout << "[Test FBX] Loaded Size: (" << (maxB.x - minB.x) << ", " << (maxB.y - minB.y) << ", " << (maxB.z - minB.z) << ")\n";

            if (mesh.isSkinned && mesh.skeleton) {
                auto anim = std::make_shared<AnimatorComponent>();
                anim->skeleton = mesh.skeleton;
                for (auto& clip : mesh.defaultClips) anim->addClip(clip);
                anim->update(0.0f);

                glm::vec3 minDef(1e9f), maxDef(-1e9f);
                for (const auto& v : mesh.vertices) {
                    glm::mat4 bt(0.0f);
                    if (v.boneIDs.x >= 0 && v.boneIDs.x < 128) bt += anim->finalBoneMatrices[v.boneIDs.x] * v.weights.x;
                    if (v.boneIDs.y >= 0 && v.boneIDs.y < 128) bt += anim->finalBoneMatrices[v.boneIDs.y] * v.weights.y;
                    if (v.boneIDs.z >= 0 && v.boneIDs.z < 128) bt += anim->finalBoneMatrices[v.boneIDs.z] * v.weights.z;
                    if (v.boneIDs.w >= 0 && v.boneIDs.w < 128) bt += anim->finalBoneMatrices[v.boneIDs.w] * v.weights.w;

                    glm::vec4 dp = bt * glm::vec4(v.pos, 1.0f);
                    minDef = glm::min(minDef, glm::vec3(dp));
                    maxDef = glm::max(maxDef, glm::vec3(dp));
                }
                std::cout << "[Test FBX] Rest Pose Deformed MinB: (" << minDef.x << ", " << minDef.y << ", " << minDef.z << ")\n";
                std::cout << "[Test FBX] Rest Pose Deformed MaxB: (" << maxDef.x << ", " << maxDef.y << ", " << maxDef.z << ")\n";
                std::cout << "[Test FBX] Rest Pose Deformed Size: (" << (maxDef.x - minDef.x) << ", " << (maxDef.y - minDef.y) << ", " << (maxDef.z - minDef.z) << ")\n";
                // Test loading external animation clips
                for (const std::string& clipFile : { "rifle aiming idle.fbx", "firing rifle.fbx" }) {
                    std::string animClipPath = "C:/Users/nguye/Downloads/Basic Shooter Pack/" + clipFile;
                    if (std::filesystem::exists(animClipPath)) {
                        auto extClip = AnimationLoader::loadFBXClip(animClipPath, mesh.skeleton.get());
                        if (extClip) {
                            std::cout << "[Test FBX] Successfully loaded external clip: " << extClip->name
                                      << " ('" << clipFile << "'), duration=" << extClip->duration << "s, tracks=" << extClip->tracks.size() << "\n";
                            anim->addClip(extClip);
                            anim->play(extClip->name, true);
                            anim->update(0.5f);

                            int rIdx = mesh.skeleton->findBoneIndex("CC_Base_R_Hand");
                            int lIdx = mesh.skeleton->findBoneIndex("CC_Base_L_Hand");
                            if (rIdx >= 0 && lIdx >= 0) {
                                glm::vec3 posR = glm::vec3(anim->boneWorldTransforms[rIdx][3]);
                                glm::vec3 posL = glm::vec3(anim->boneWorldTransforms[lIdx][3]);
                                glm::vec3 forward = posL - posR;
                                float dist = glm::length(forward);
                                std::cout << "  -> posR (Right Hand): (" << posR.x << ", " << posR.y << ", " << posR.z << ")\n";
                                std::cout << "  -> posL (Left Hand): (" << posL.x << ", " << posL.y << ", " << posL.z << ")\n";
                                std::cout << "  -> Hand Distance: " << dist << "m\n";

                                glm::vec3 fwdNorm = glm::normalize(forward);
                                glm::vec3 refUp = glm::vec3(0.0f, 1.0f, 0.0f);
                                glm::vec3 up = glm::normalize(refUp - glm::dot(refUp, fwdNorm) * fwdNorm);
                                glm::vec3 right = glm::cross(fwdNorm, up);
                                glm::mat4 rot2H(1.0f);
                                rot2H[0] = glm::vec4(right, 0.0f);
                                rot2H[1] = glm::vec4(-fwdNorm, 0.0f);
                                rot2H[2] = glm::vec4(-up, 0.0f);
                                glm::vec3 gripOffset(0.0f, 0.070f, 0.080f);
                                glm::vec3 gunOrigin = posR - glm::vec3(rot2H * glm::vec4(gripOffset, 0.0f));
                                rot2H[3] = glm::vec4(gunOrigin, 1.0f);
                                std::cout << "  -> Gun Origin: (" << gunOrigin.x << ", " << gunOrigin.y << ", " << gunOrigin.z << ")\n";
                                std::cout << "  -> Muzzle Pos: (" << (gunOrigin - fwdNorm * 0.5856f).x << ", " << (gunOrigin - fwdNorm * 0.5856f).y << ", " << (gunOrigin - fwdNorm * 0.5856f).z << ")\n";
                            }
                        }
                    }
                }
            }
            return EXIT_SUCCESS;
        } catch (const std::exception& e) {
            std::cerr << "[Test FBX ERROR] " << e.what() << "\n";
            return EXIT_FAILURE;
        }
    }

    VulkanApp app;

    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}