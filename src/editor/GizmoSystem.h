#pragma once

#include "core/Types.h"
#include <glm/glm.hpp>
#include "imgui.h"
#include <functional>

class Scene;
class AssetManager;

class GizmoSystem
{
public:
    GizmoSystem();

    void drawSceneView(Scene* scene, AssetManager* assetManager, VkDescriptorSet offscreenTexture,
                       const ImVec2& windowPos, const ImVec2& windowSize, AppMode mode);
    void drawGameView(VkDescriptorSet gameViewTexture, const ImVec2& windowPos, const ImVec2& windowSize,
                      int gameScore, int highScore, std::function<void()> onToggleFullscreen,
                      AppMode mode = AppMode::EDIT);
    void draw3DObject(const Scene& scene, int objIndex, const glm::mat4& view, const glm::mat4& proj,
                      const ImVec2& offset, const ImVec2& size, ImU32 customColor = 0, float thickness = 1.5f);

    // Camera Framing & Focus helper (Blender / Unity / Maya style)
    void focusOnObject(int objIndex, const Scene& scene);

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
    float sceneRotationX = 25.0f;
    float sceneRotationY = 45.0f;
    float sceneCameraDistance = 5.0f;
    glm::vec3 sceneCameraTarget = glm::vec3(0.0f);

    glm::vec3 mainCameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 mainCameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float mainCameraFov = 45.0f;
    float mainCameraNear = 0.1f;
    float mainCameraFar = 20.0f;

    bool showGameViewWindow = true;
    bool isGameViewDetached = false;
    bool isGameFullscreen = false;

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
