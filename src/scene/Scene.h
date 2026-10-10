#pragma once

#include "scene/SceneObject.h"
#include <vector>
#include <string>

class PhysEngine;
class AssetManager;
class GizmoSystem;
class VulkanRenderer;

struct CameraSettings {
    // Scene View Orbit Camera
    float sceneRotationX = 25.0f;
    float sceneRotationY = 45.0f;
    float sceneCameraDistance = 5.0f;
    glm::vec3 sceneCameraTarget = glm::vec3(0.0f);

    // Main / Game / Photo View Camera
    glm::vec3 mainCameraPos = glm::vec3(0.0f, 3.5f, 9.5f);
    glm::vec3 mainCameraTarget = glm::vec3(0.0f, 1.0f, 0.0f);
    float mainCameraFov = 45.0f;
    float mainCameraNear = 0.1f;
    float mainCameraFar = 100.0f;
    int aspectRatioMode = 0; // 0 = PHOTO_2M4_X_2M, 1 = RATIO_16_9, 2 = FREE_FILL
    bool hasSavedCamera = false;

    // Photo View Frame Settings
    float photoFrameWidthMeters = 2.4f;
    float photoFrameHeightMeters = 2.0f;
    bool showPhotoGrid = false;
    bool showPhotoFrameRulers = true;

    // Renderer Settings
    bool enableShadows = true;
};

class Scene
{
public:
    Scene();

    std::vector<SceneObject>& getObjects() { return sceneObjects; }
    const std::vector<SceneObject>& getObjects() const { return sceneObjects; }

    int getSelectedObjectIndex() const { return selectedObjectIndex; }
    void setSelectedObjectIndex(int index) { selectedObjectIndex = index; }

    SceneObject* getSelectedObject();
    const SceneObject* getSelectedObject() const;

    glm::mat4 getWorldMatrix(int index) const;
    static glm::mat4 calculateWorldMatrix(const std::vector<SceneObject>& objects, int index);

    void saveHistory();
    void undo();
    void redo();

    void savePlayModeState();
    void restoreEditModeState(PhysEngine* physEngine);

    void captureCameraSettings(const GizmoSystem* gizmo, const VulkanRenderer* renderer = nullptr);
    void applyCameraSettings(GizmoSystem* gizmo, VulkanRenderer* renderer = nullptr);

    const CameraSettings& getCameraSettings() const { return cameraSettings; }
    CameraSettings& getCameraSettings() { return cameraSettings; }

    void saveScene(const std::string& filename, const AssetManager* assetManager = nullptr, const GizmoSystem* gizmo = nullptr, const VulkanRenderer* renderer = nullptr);
    bool exportToJMonkeyEngine(const std::string& filename = "scene_jme.json", const AssetManager* assetManager = nullptr, const GizmoSystem* gizmo = nullptr, const VulkanRenderer* renderer = nullptr);
    void loadScene(const std::string& filename, AssetManager* assetManager = nullptr, int cubeMeshId = -1, int sphereMeshId = -1, int planeMeshId = -1, GizmoSystem* gizmo = nullptr, VulkanRenderer* renderer = nullptr, PhysEngine* physEngine = nullptr);
    void loadScene(const std::string& filename, int cubeMeshId, int sphereMeshId, int planeMeshId);

    void initializeDefaultScene(int cubeMeshId, int sphereMeshId, int planeMeshId, int groundTexId, PhysEngine* physEngine);

    glm::vec3 getPlayerStartPos() const { return playerStartPos; }
    void setPlayerStartPos(const glm::vec3& pos) { playerStartPos = pos; }

    void initPhysicsBodies(PhysEngine* physEngine);
    void syncPhysicsToTransform(PhysEngine* physEngine);

    const std::string& getCurrentScenePath() const { return currentScenePath; }
    void setCurrentScenePath(const std::string& path) { currentScenePath = path; }

    const std::string& getCurrentEnvironmentName() const { return currentEnvironmentName; }
    void setCurrentEnvironmentName(const std::string& name) { currentEnvironmentName = name; }

    static std::string getEnvironmentFileName(const std::string& envName);

private:
    std::vector<SceneObject> sceneObjects;
    int selectedObjectIndex = 0;

    std::vector<std::vector<SceneObject>> undoStack;
    std::vector<std::vector<SceneObject>> redoStack;

    glm::vec3 playerStartPos = glm::vec3(0.0f);
    CameraSettings cameraSettings;

    std::string currentScenePath = "scene.json";
    std::string currentEnvironmentName = "Forest Clearing";
};
