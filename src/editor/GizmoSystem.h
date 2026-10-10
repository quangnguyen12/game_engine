#pragma once

#include "core/Types.h"
#include <glm/glm.hpp>
#include "imgui.h"
#include <functional>

class Scene;
class AssetManager;
class VulkanRenderer;

enum class GameViewAspectRatioMode
{
    PHOTO_2M4_X_2M = 0, // 2.4m x 2.0m (Tỉ lệ 1.2:1 / 6:5) - Chuẩn Màn Chiếu / View Ảnh
    RATIO_16_9 = 1,     // 16:9 Chuẩn Màn Hình
    FREE_FILL = 2       // Tự do kéo giãn theo cửa sổ
};

class GizmoSystem
{
public:
    GizmoSystem();

    void drawSceneView(Scene* scene, AssetManager* assetManager, VkDescriptorSet offscreenTexture,
                       const ImVec2& windowPos, const ImVec2& windowSize, AppMode mode);
    void drawGameView(VkDescriptorSet gameViewTexture, const ImVec2& windowPos, const ImVec2& windowSize,
                      int gameScore, int highScore, std::function<void()> onToggleFullscreen,
                      AppMode mode = AppMode::EDIT, const Scene* scene = nullptr,
                      VulkanRenderer* renderer = nullptr);
    void draw3DObject(const Scene& scene, int objIndex, const glm::mat4& view, const glm::mat4& proj,
                      const ImVec2& offset, const ImVec2& size, ImU32 customColor = 0, float thickness = 1.5f);

    // Camera Framing & Focus helper (Blender / Unity / Maya style)
    void focusOnObject(int objIndex, const Scene& scene);
    void frameScenicPhotoView(const Scene& scene, int viewAngleType = 0);
    float getGameViewAspectRatio() const;

    // Math & Projection helpers
    static ImVec2 projectPoint(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj,
                               const ImVec2& offset, const ImVec2& size);
    static bool getRayFromScreenPos(const ImVec2& mousePos, const ImVec2& windowPos, const ImVec2& windowSize,
                                    const glm::mat4& view, const glm::mat4& proj,
                                    glm::vec3& rayOrigin, glm::vec3& rayDir);
    static bool intersectRayPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                  const glm::vec3& planePoint, const glm::vec3& planeNormal,
                                  glm::vec3& hitPoint);
    static float getClosestPointOnAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                       const glm::vec3& pivotPos, const glm::vec3& axisDir,
                                       const glm::vec3& cameraPos);

    // Camera parameters
    float sceneRotationX = 20.0f;
    float sceneRotationY = 40.0f;
    float sceneCameraDistance = 14.0f;
    glm::vec3 sceneCameraTarget = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 mainCameraPos = glm::vec3(0.0f, 3.2f, 14.5f);
    glm::vec3 mainCameraTarget = glm::vec3(0.0f, 1.2f, 0.0f);
    float mainCameraFov = 45.0f;
    float mainCameraNear = 0.1f;
    float mainCameraFar = 120.0f;

    // Photo View (Khung hình 2.4m x 2.0m) settings
    GameViewAspectRatioMode gameAspectRatioMode = GameViewAspectRatioMode::PHOTO_2M4_X_2M;
    bool showPhotoGrid = false;
    bool showPhotoFrameRulers = true;
    float photoFrameWidthMeters = 2.4f;
    float photoFrameHeightMeters = 2.0f;

    bool showGameViewWindow = true;
    bool isGameViewDetached = false;
    bool isGameFullscreen = false;

    // Screenshot & Image Export State
    std::string lastExportedImagePath = "";
    float exportNotificationTimer = 0.0f;
    int exportResolutionPreset = 0; // 0: 2400x2000 (Ultra HD), 1: 1200x1000 (Standard), 2: 1280x720 (Native)
    bool showExportPopup = false;

    // Gizmo state
    GizmoType activeGizmo = GizmoType::TRANSLATE;
    DragAxis activeDragAxis = DragAxis::NONE;
    DragAxis hoveredDragAxis = DragAxis::NONE;
    GizmoDragState gizmoDragState;

    bool isDraggingObject = false;
    bool wasDraggingObjectLastFrame = false;
    bool isBlenderGrabMode = false;
    glm::vec3 grabStartPos = glm::vec3(0.0f);
    int grabConstrainAxis = -1; // -1: free XZ ground plane, 0: X, 1: Y, 2: Z
};
