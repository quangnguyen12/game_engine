#include "editor/GizmoSystem.h"
#include "scene/Scene.h"
#include "assets/AssetManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>

GizmoSystem::GizmoSystem()
{
}

ImVec2 GizmoSystem::projectPoint(const glm::vec3& p, const glm::mat4& view, const glm::mat4& proj,
                                 const ImVec2& offset, const ImVec2& size)
{
    glm::vec4 clipSpacePos = proj * view * glm::vec4(p, 1.0f);
    if (clipSpacePos.w <= 0.0f) return ImVec2(-99999.0f, -99999.0f);

    glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;

    float x = offset.x + (ndcSpacePos.x + 1.0f) * 0.5f * size.x;
    float y = offset.y + (1.0f - ndcSpacePos.y) * 0.5f * size.y;
    return ImVec2(x, y);
}

bool GizmoSystem::getRayFromScreenPos(const ImVec2& mousePos, const ImVec2& windowPos, const ImVec2& windowSize,
                                      const glm::mat4& view, const glm::mat4& proj,
                                      glm::vec3& rayOrigin, glm::vec3& rayDir)
{
    if (windowSize.x <= 0.0f || windowSize.y <= 0.0f) return false;

    float mouseRelX = mousePos.x - windowPos.x;
    float mouseRelY = mousePos.y - windowPos.y;

    float ndcX = (mouseRelX / windowSize.x) * 2.0f - 1.0f;
    float ndcY = 1.0f - (mouseRelY / windowSize.y) * 2.0f;

    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 nearNDC(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farNDC(ndcX, ndcY, 1.0f, 1.0f);

    glm::vec4 nearWorld = invVP * nearNDC;
    if (std::abs(nearWorld.w) < 1e-6f) return false;
    nearWorld /= nearWorld.w;

    glm::vec4 farWorld = invVP * farNDC;
    if (std::abs(farWorld.w) < 1e-6f) return false;
    farWorld /= farWorld.w;

    rayOrigin = glm::vec3(nearWorld);
    rayDir = glm::normalize(glm::vec3(farWorld - nearWorld));
    return true;
}

static bool intersectRaySphere(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                               const glm::vec3& center, float radius, float& hitDist)
{
    glm::vec3 oc = rayOrigin - center;
    float b = glm::dot(oc, rayDir);
    float c = glm::dot(oc, oc) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0f) return false;
    float sqrtDisc = std::sqrt(disc);
    float t = -b - sqrtDisc;
    if (t < 0.0f) t = -b + sqrtDisc;
    if (t < 0.0f) return false;
    hitDist = t;
    return true;
}

bool GizmoSystem::intersectRayPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                    const glm::vec3& planePoint, const glm::vec3& planeNormal,
                                    glm::vec3& hitPoint)
{
    float denom = glm::dot(rayDir, planeNormal);
    if (std::abs(denom) < 1e-6f) return false;

    float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    if (t < 0.0f) return false;

    hitPoint = rayOrigin + t * rayDir;
    return true;
}

float GizmoSystem::getClosestPointOnAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                        const glm::vec3& pivotPos, const glm::vec3& axisDir,
                                        const glm::vec3& cameraPos)
{
    glm::vec3 a = glm::normalize(axisDir);
    glm::vec3 camDir = cameraPos - pivotPos;
    if (glm::length(camDir) < 0.001f) camDir = glm::vec3(0.0f, 0.0f, 1.0f);
    else camDir = glm::normalize(camDir);

    glm::vec3 side = glm::cross(a, camDir);
    if (glm::length(side) < 0.001f)
    {
        side = glm::cross(a, glm::vec3(0.0f, 0.0f, 1.0f));
        if (glm::length(side) < 0.001f)
            side = glm::cross(a, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    if (glm::length(side) < 0.001f)
    {
        side = glm::vec3(1.0f, 0.0f, 0.0f);
    }
    side = glm::normalize(side);
    glm::vec3 planeNormal = glm::cross(side, a);
    if (glm::length(planeNormal) < 0.001f)
    {
        planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    }
    else
    {
        planeNormal = glm::normalize(planeNormal);
    }

    glm::vec3 hitPoint;
    if (intersectRayPlane(rayOrigin, rayDir, pivotPos, planeNormal, hitPoint))
    {
        return glm::dot(hitPoint - pivotPos, a);
    }
    return 0.0f;
}

void GizmoSystem::drawSceneView(Scene* scene, AssetManager* assetManager, VkDescriptorSet offscreenTexture,
                                const ImVec2& windowPos, const ImVec2& windowSize, AppMode mode)
{
    if (!scene) return;
    auto& sceneObjects = scene->getObjects();
    int selectedObjectIndex = scene->getSelectedObjectIndex();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Draw background
    drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), IM_COL32(40, 40, 40, 255));

    // Calculate Camera Matrices
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), windowSize.x / (windowSize.y > 0.0f ? windowSize.y : 1.0f), 0.1f, 100.0f);
    glm::vec3 offset(
        sceneCameraDistance * cos(glm::radians(sceneRotationX)) * sin(glm::radians(sceneRotationY)),
        sceneCameraDistance * sin(glm::radians(sceneRotationX)),
        sceneCameraDistance * cos(glm::radians(sceneRotationX)) * cos(glm::radians(sceneRotationY))
    );
    glm::vec3 sceneCamPos = sceneCameraTarget + offset;
    glm::mat4 view = glm::lookAt(sceneCamPos, sceneCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 invView = glm::inverse(view);
    glm::vec3 cameraWorldPos = glm::vec3(invView[3]);
    glm::vec3 cameraForward = glm::normalize(glm::vec3(-invView[2]));

    // Tool Overlay & Active Drag Status Banner
    bool hasSelection = (selectedObjectIndex >= -1 && selectedObjectIndex < static_cast<int>(sceneObjects.size()));
    float toolWidth = (gizmoDragState.isDragging && hasSelection) ? 460.0f : 360.0f;
    drawList->AddRectFilled(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + toolWidth, windowPos.y + 35), IM_COL32(20, 20, 25, 230), 4.0f);
    drawList->AddRect(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + toolWidth, windowPos.y + 35), gizmoDragState.isDragging ? IM_COL32(255, 200, 50, 255) : IM_COL32(100, 100, 100, 255), 4.0f);

    char toolStr[256];
    const char* toolNames[] = { "HAND", "TRANSLATE", "ROTATE", "SCALE", "RECT", "COMBINED" };
    if (gizmoDragState.isDragging && hasSelection)
    {
        std::string objName = (selectedObjectIndex == -1) ? "Main Camera" : sceneObjects[selectedObjectIndex].name;
        const char* axisNames[] = { "NONE", "X Axis", "Y Axis", "Z Axis", "XY Plane", "YZ Plane", "XZ Plane", "Free Plane" };
        snprintf(toolStr, sizeof(toolStr), "Tool: %s | Dragging: %s [%s]", toolNames[static_cast<int>(activeGizmo)], objName.c_str(), axisNames[static_cast<int>(gizmoDragState.axis)]);
    }
    else
    {
        snprintf(toolStr, sizeof(toolStr), "Tool: %s  [Q/W/E/R/T/Y]", toolNames[static_cast<int>(activeGizmo)]);
    }
    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 15), IM_COL32(255, 255, 255, 255), toolStr);

    // Get selected object's 3D position
    glm::vec3 pivotPos(0.0f);
    if (hasSelection)
    {
        pivotPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
    }

    // Project selected object center and axis handles
    ImVec2 sP(-99999.0f, -99999.0f);
    ImVec2 sX(-99999.0f, -99999.0f);
    ImVec2 sY(-99999.0f, -99999.0f);
    ImVec2 sZ(-99999.0f, -99999.0f);
    ImVec2 sPlaneX(-99999.0f, -99999.0f);
    ImVec2 sPlaneZ(-99999.0f, -99999.0f);
    ImVec2 sXZ(-99999.0f, -99999.0f);

    float L = std::clamp(0.18f * sceneCameraDistance, 0.8f, 3.5f);
    float planeOffset = L * 0.35f;
    if (hasSelection)
    {
        sP = projectPoint(pivotPos, view, proj, windowPos, windowSize);
        sX = projectPoint(pivotPos + glm::vec3(L, 0.0f, 0.0f), view, proj, windowPos, windowSize);
        sY = projectPoint(pivotPos + glm::vec3(0.0f, L, 0.0f), view, proj, windowPos, windowSize);
        sZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, L), view, proj, windowPos, windowSize);
        sPlaneX = projectPoint(pivotPos + glm::vec3(planeOffset, 0.0f, 0.0f), view, proj, windowPos, windowSize);
        sPlaneZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, planeOffset), view, proj, windowPos, windowSize);
        sXZ = projectPoint(pivotPos + glm::vec3(planeOffset, 0.0f, planeOffset), view, proj, windowPos, windowSize);
    }

    ImVec2 mousePos = ImGui::GetMousePos();
    bool isMouseInWindow = ImGui::IsWindowHovered();

    // 1. ACTIVE DRAGGING EXECUTION
    if (gizmoDragState.isDragging)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && hasSelection)
        {
            glm::vec3 curRayOrig, curRayDir;
            if (getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, curRayOrig, curRayDir))
            {
                glm::vec3 dummyRot(0.0f), dummyScale(1.0f);
                glm::vec3& posRef = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
                glm::vec3& rotRef = (selectedObjectIndex == -1) ? dummyRot : sceneObjects[selectedObjectIndex].rotation;
                glm::vec3& scaleRef = (selectedObjectIndex == -1) ? dummyScale : sceneObjects[selectedObjectIndex].scale;

                if (gizmoDragState.gizmoType == GizmoType::TRANSLATE || gizmoDragState.gizmoType == GizmoType::TRANSFORM_COMBINED)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Y || gizmoDragState.axis == DragAxis::Z)
                    {
                        float curAxisVal = getClosestPointOnAxis(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.axisDir, cameraWorldPos);
                        float delta = curAxisVal - gizmoDragState.startAxisVal;
                        posRef = gizmoDragState.startObjPos + gizmoDragState.axisDir * delta;
                    }
                    else if (gizmoDragState.axis == DragAxis::XZ)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, glm::vec3(0.0f, 1.0f, 0.0f), curHit))
                        {
                            glm::vec3 delta = curHit - gizmoDragState.startHitPoint;
                            posRef = gizmoDragState.startObjPos + glm::vec3(delta.x, 0.0f, delta.z);
                        }
                    }
                    else if (gizmoDragState.axis == DragAxis::FREE)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.planeNormal, curHit))
                        {
                            glm::vec3 delta = curHit - gizmoDragState.startHitPoint;
                            posRef = gizmoDragState.startObjPos + delta;
                        }
                    }
                }
                else if (gizmoDragState.gizmoType == GizmoType::ROTATE)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Y || gizmoDragState.axis == DragAxis::Z)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.planeNormal, curHit))
                        {
                            glm::vec3 dirVec = curHit - gizmoDragState.pivotPos;
                            if (glm::length(dirVec) > 0.001f)
                            {
                                float curAngle = atan2(glm::dot(dirVec, gizmoDragState.rotBasisV), glm::dot(dirVec, gizmoDragState.rotBasisU));
                                float deltaAngle = curAngle - gizmoDragState.startAngle;
                                while (deltaAngle > glm::pi<float>()) deltaAngle -= glm::two_pi<float>();
                                while (deltaAngle < -glm::pi<float>()) deltaAngle -= glm::two_pi<float>();

                                rotRef = gizmoDragState.startObjRot + gizmoDragState.axisDir * glm::degrees(deltaAngle);
                            }
                        }
                    }
                }
                else if (gizmoDragState.gizmoType == GizmoType::SCALE)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Y || gizmoDragState.axis == DragAxis::Z)
                    {
                        float curAxisVal = getClosestPointOnAxis(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.axisDir, cameraWorldPos);
                        float delta = curAxisVal - gizmoDragState.startAxisVal;
                        int idx = (gizmoDragState.axis == DragAxis::X) ? 0 : (gizmoDragState.axis == DragAxis::Y) ? 1 : 2;
                        scaleRef = gizmoDragState.startObjScale;
                        scaleRef[idx] = glm::max(0.01f, gizmoDragState.startObjScale[idx] + delta / L);
                    }
                    else if (gizmoDragState.axis == DragAxis::FREE)
                    {
                        glm::vec3 curHit;
                        if (intersectRayPlane(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.planeNormal, curHit))
                        {
                            float curDist = glm::distance(curHit, gizmoDragState.pivotPos);
                            float scaleFactor = (gizmoDragState.startDist > 0.001f) ? (curDist / gizmoDragState.startDist) : 1.0f;
                            scaleRef = glm::max(glm::vec3(0.01f), gizmoDragState.startObjScale * scaleFactor);
                        }
                    }
                }
                else if (gizmoDragState.gizmoType == GizmoType::RECT)
                {
                    if (gizmoDragState.axis == DragAxis::X || gizmoDragState.axis == DragAxis::Z)
                    {
                        float curAxisVal = getClosestPointOnAxis(curRayOrig, curRayDir, gizmoDragState.pivotPos, gizmoDragState.axisDir, cameraWorldPos);
                        float delta = curAxisVal - gizmoDragState.startAxisVal;
                        int idx = (gizmoDragState.axis == DragAxis::X) ? 0 : 2;
                        scaleRef = gizmoDragState.startObjScale;
                        scaleRef[idx] = glm::max(0.01f, gizmoDragState.startObjScale[idx] + delta / L);
                    }
                }

                // Snap support when Ctrl is held down
                if (ImGui::GetIO().KeyCtrl && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    auto& obj = sceneObjects[selectedObjectIndex];
                    if (activeGizmo == GizmoType::TRANSLATE) {
                        obj.position.x = std::round(obj.position.x * 2.0f) / 2.0f;
                        obj.position.y = std::round(obj.position.y * 2.0f) / 2.0f;
                        obj.position.z = std::round(obj.position.z * 2.0f) / 2.0f;
                    } else if (activeGizmo == GizmoType::ROTATE) {
                        obj.rotation.x = std::round(obj.rotation.x / 15.0f) * 15.0f;
                        obj.rotation.y = std::round(obj.rotation.y / 15.0f) * 15.0f;
                        obj.rotation.z = std::round(obj.rotation.z / 15.0f) * 15.0f;
                    } else if (activeGizmo == GizmoType::SCALE || activeGizmo == GizmoType::RECT) {
                        obj.scale.x = std::round(obj.scale.x / 0.1f) * 0.1f;
                        obj.scale.y = std::round(obj.scale.y / 0.1f) * 0.1f;
                        obj.scale.z = std::round(obj.scale.z / 0.1f) * 0.1f;
                    }
                }

                if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    sceneObjects[selectedObjectIndex].syncComponents();
                }

                // Re-update pivotPos
                if (hasSelection)
                {
                    pivotPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
                    sP = projectPoint(pivotPos, view, proj, windowPos, windowSize);
                    sX = projectPoint(pivotPos + glm::vec3(L, 0.0f, 0.0f), view, proj, windowPos, windowSize);
                    sY = projectPoint(pivotPos + glm::vec3(0.0f, L, 0.0f), view, proj, windowPos, windowSize);
                    sZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, L), view, proj, windowPos, windowSize);
                    sPlaneX = projectPoint(pivotPos + glm::vec3(planeOffset, 0.0f, 0.0f), view, proj, windowPos, windowSize);
                    sPlaneZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, planeOffset), view, proj, windowPos, windowSize);
                    sXZ = projectPoint(pivotPos + glm::vec3(planeOffset, 0.0f, planeOffset), view, proj, windowPos, windowSize);
                }
            }
        }
        else
        {
            gizmoDragState.isDragging = false;
            gizmoDragState.axis = DragAxis::NONE;
            activeDragAxis = DragAxis::NONE;
            isDraggingObject = false;
        }
    }

    // 1b. BLENDER GRAB MODE (G hotkey)
    if (isBlenderGrabMode && hasSelection)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            if (selectedObjectIndex == -1) mainCameraPos = grabStartPos;
            else sceneObjects[selectedObjectIndex].position = grabStartPos;
            if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                sceneObjects[selectedObjectIndex].syncComponents();
            isBlenderGrabMode = false;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space) || ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            isBlenderGrabMode = false;
        }
        else
        {
            if (ImGui::IsKeyPressed(ImGuiKey_X)) grabConstrainAxis = (grabConstrainAxis == 0) ? -1 : 0;
            if (ImGui::IsKeyPressed(ImGuiKey_Y)) grabConstrainAxis = (grabConstrainAxis == 1) ? -1 : 1;
            if (ImGui::IsKeyPressed(ImGuiKey_Z)) grabConstrainAxis = (grabConstrainAxis == 2) ? -1 : 2;

            glm::vec3 curRayOrig, curRayDir;
            if (getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, curRayOrig, curRayDir))
            {
                glm::vec3& posRef = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
                if (grabConstrainAxis == 0)
                {
                    float curVal = getClosestPointOnAxis(curRayOrig, curRayDir, grabStartPos, glm::vec3(1.0f, 0.0f, 0.0f), cameraWorldPos);
                    posRef = glm::vec3(curVal, grabStartPos.y, grabStartPos.z);
                }
                else if (grabConstrainAxis == 1)
                {
                    float curVal = getClosestPointOnAxis(curRayOrig, curRayDir, grabStartPos, glm::vec3(0.0f, 1.0f, 0.0f), cameraWorldPos);
                    posRef = glm::vec3(grabStartPos.x, curVal, grabStartPos.z);
                }
                else if (grabConstrainAxis == 2)
                {
                    float curVal = getClosestPointOnAxis(curRayOrig, curRayDir, grabStartPos, glm::vec3(0.0f, 0.0f, 1.0f), cameraWorldPos);
                    posRef = glm::vec3(grabStartPos.x, grabStartPos.y, curVal);
                }
                else
                {
                    glm::vec3 hitPoint;
                    if (intersectRayPlane(curRayOrig, curRayDir, grabStartPos, glm::vec3(0.0f, 1.0f, 0.0f), hitPoint))
                    {
                        posRef = glm::vec3(hitPoint.x, grabStartPos.y, hitPoint.z);
                    }
                }

                if (ImGui::GetIO().KeyCtrl)
                {
                    posRef.x = std::round(posRef.x * 2.0f) / 2.0f;
                    posRef.y = std::round(posRef.y * 2.0f) / 2.0f;
                    posRef.z = std::round(posRef.z * 2.0f) / 2.0f;
                }

                if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                    sceneObjects[selectedObjectIndex].syncComponents();

                pivotPos = posRef;
                sP = projectPoint(pivotPos, view, proj, windowPos, windowSize);
                sX = projectPoint(pivotPos + glm::vec3(L, 0.0f, 0.0f), view, proj, windowPos, windowSize);
                sY = projectPoint(pivotPos + glm::vec3(0.0f, L, 0.0f), view, proj, windowPos, windowSize);
                sZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, L), view, proj, windowPos, windowSize);
                sPlaneX = projectPoint(pivotPos + glm::vec3(planeOffset, 0.0f, 0.0f), view, proj, windowPos, windowSize);
                sPlaneZ = projectPoint(pivotPos + glm::vec3(0.0f, 0.0f, planeOffset), view, proj, windowPos, windowSize);
                sXZ = projectPoint(pivotPos + glm::vec3(planeOffset, 0.0f, planeOffset), view, proj, windowPos, windowSize);
            }
        }
    }

    // 2. HOVER DETECTION AND DRAG INITIALIZATION
    hoveredDragAxis = DragAxis::NONE;
    int hoveredSceneObjIdx = -2;

    glm::vec3 hRayOrig, hRayDir;
    bool hasHoverRay = getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, hRayOrig, hRayDir);

    if (hasHoverRay && isMouseInWindow && !gizmoDragState.isDragging && !isBlenderGrabMode)
    {
        float closestHitT = 1e9f;

        float camHitT = 0.0f;
        if (intersectRaySphere(hRayOrig, hRayDir, mainCameraPos, 0.8f, camHitT))
        {
            if (camHitT < closestHitT)
            {
                closestHitT = camHitT;
                hoveredSceneObjIdx = -1;
            }
        }

        for (size_t i = 0; i < sceneObjects.size(); ++i)
        {
            const auto& obj = sceneObjects[i];
            float maxScale = std::max({ std::abs(obj.scale.x), std::abs(obj.scale.y), std::abs(obj.scale.z) });
            float radius = std::max(0.8f, maxScale * 0.9f);
            glm::vec3 center = obj.position;

            std::string lowerName = obj.name;
            for (char& c : lowerName) c = (char)std::tolower((unsigned char)c);

            if (lowerName.find("tree") != std::string::npos)
            {
                radius = std::max(1.2f, 2.2f * maxScale);
                center.y += radius * 0.6f;
            }
            else if (lowerName.find("rock") != std::string::npos)
            {
                radius = std::max(0.8f, 1.6f * maxScale);
                center.y += radius * 0.3f;
            }
            else if (lowerName.find("house") != std::string::npos || lowerName.find("building") != std::string::npos)
            {
                radius = std::max(1.5f, 3.2f * maxScale);
                center.y += radius * 0.5f;
            }

            float hitT = 0.0f;
            if (intersectRaySphere(hRayOrig, hRayDir, center, radius, hitT))
            {
                if (hitT < closestHitT)
                {
                    closestHitT = hitT;
                    hoveredSceneObjIdx = static_cast<int>(i);
                }
            }
        }
    }

    // Check Gizmo Axis and Plane Hover
    if (hasSelection && sP.x > -90000.0f && activeGizmo != GizmoType::HAND && !gizmoDragState.isDragging && !isBlenderGrabMode)
    {
        auto distToSeg = [](glm::vec2 p, glm::vec2 a, glm::vec2 b) -> float {
            glm::vec2 ab = b - a;
            float len2 = glm::dot(ab, ab);
            if (len2 < 0.001f) return glm::distance(p, a);
            float t = glm::clamp(glm::dot(p - a, ab) / len2, 0.0f, 1.0f);
            glm::vec2 proj = a + t * ab;
            return glm::distance(p, proj);
        };

        glm::vec2 mPos(mousePos.x, mousePos.y);
        float distCenter = glm::distance(mPos, glm::vec2(sP.x, sP.y));
        float distX = (sX.x > -90000.0f) ? distToSeg(mPos, glm::vec2(sP.x, sP.y), glm::vec2(sX.x, sX.y)) : 99999.0f;
        float distY = (sY.x > -90000.0f) ? distToSeg(mPos, glm::vec2(sP.x, sP.y), glm::vec2(sY.x, sY.y)) : 99999.0f;
        float distZ = (sZ.x > -90000.0f) ? distToSeg(mPos, glm::vec2(sP.x, sP.y), glm::vec2(sZ.x, sZ.y)) : 99999.0f;
        float distXZ = (sXZ.x > -90000.0f) ? glm::distance(mPos, glm::vec2(sXZ.x, sXZ.y)) : 99999.0f;

        if (activeGizmo == GizmoType::TRANSLATE || activeGizmo == GizmoType::TRANSFORM_COMBINED)
        {
            if (distCenter < 14.0f) hoveredDragAxis = DragAxis::FREE;
            else if (distXZ < 16.0f) hoveredDragAxis = DragAxis::XZ;
            else if (distX < 12.0f) hoveredDragAxis = DragAxis::X;
            else if (distY < 12.0f) hoveredDragAxis = DragAxis::Y;
            else if (distZ < 12.0f) hoveredDragAxis = DragAxis::Z;
        }
        else if (activeGizmo == GizmoType::SCALE)
        {
            if (distCenter < 14.0f) hoveredDragAxis = DragAxis::FREE;
            else if (distX < 12.0f) hoveredDragAxis = DragAxis::X;
            else if (distY < 12.0f) hoveredDragAxis = DragAxis::Y;
            else if (distZ < 12.0f) hoveredDragAxis = DragAxis::Z;
        }
        else if (activeGizmo == GizmoType::ROTATE)
        {
            float bestRingDist = 12.0f;
            const int numSegs = 36;
            for (int i = 0; i < numSegs; ++i)
            {
                float a1 = (i * 2.0f * 3.14159f) / numSegs;
                float a2 = ((i + 1) * 2.0f * 3.14159f) / numSegs;

                ImVec2 rx1 = projectPoint(pivotPos + glm::vec3(0.0f, cos(a1) * L, sin(a1) * L), view, proj, windowPos, windowSize);
                ImVec2 rx2 = projectPoint(pivotPos + glm::vec3(0.0f, cos(a2) * L, sin(a2) * L), view, proj, windowPos, windowSize);
                if (rx1.x > -90000.0f && rx2.x > -90000.0f) {
                    float d = distToSeg(mPos, glm::vec2(rx1.x, rx1.y), glm::vec2(rx2.x, rx2.y));
                    if (d < bestRingDist) { bestRingDist = d; hoveredDragAxis = DragAxis::X; }
                }

                ImVec2 ry1 = projectPoint(pivotPos + glm::vec3(cos(a1) * L, 0.0f, sin(a1) * L), view, proj, windowPos, windowSize);
                ImVec2 ry2 = projectPoint(pivotPos + glm::vec3(cos(a2) * L, 0.0f, sin(a2) * L), view, proj, windowPos, windowSize);
                if (ry1.x > -90000.0f && ry2.x > -90000.0f) {
                    float d = distToSeg(mPos, glm::vec2(ry1.x, ry1.y), glm::vec2(ry2.x, ry2.y));
                    if (d < bestRingDist) { bestRingDist = d; hoveredDragAxis = DragAxis::Y; }
                }

                ImVec2 rz1 = projectPoint(pivotPos + glm::vec3(cos(a1) * L, sin(a1) * L, 0.0f), view, proj, windowPos, windowSize);
                ImVec2 rz2 = projectPoint(pivotPos + glm::vec3(cos(a2) * L, sin(a2) * L, 0.0f), view, proj, windowPos, windowSize);
                if (rz1.x > -90000.0f && rz2.x > -90000.0f) {
                    float d = distToSeg(mPos, glm::vec2(rz1.x, rz1.y), glm::vec2(rz2.x, rz2.y));
                    if (d < bestRingDist) { bestRingDist = d; hoveredDragAxis = DragAxis::Z; }
                }
            }
        }
        else if (activeGizmo == GizmoType::RECT)
        {
            if (distX < 12.0f) hoveredDragAxis = DragAxis::X;
            else if (distZ < 12.0f) hoveredDragAxis = DragAxis::Z;
        }
    }

    // Set cursor feedback
    if (hoveredDragAxis != DragAxis::NONE || gizmoDragState.isDragging)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }
    else if (hoveredSceneObjIdx != -2 && !isBlenderGrabMode)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // Handle mouse click to start gizmo drag or select & drag object directly
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && isMouseInWindow && !gizmoDragState.isDragging && !isBlenderGrabMode)
    {
        if (hoveredDragAxis != DragAxis::NONE && hasSelection)
        {
            scene->saveHistory();
            gizmoDragState.isDragging = true;
            gizmoDragState.axis = hoveredDragAxis;
            gizmoDragState.gizmoType = activeGizmo;
            activeDragAxis = hoveredDragAxis;
            isDraggingObject = true;

            gizmoDragState.startObjPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
            gizmoDragState.startObjRot = (selectedObjectIndex == -1) ? glm::vec3(0.0f) : sceneObjects[selectedObjectIndex].rotation;
            gizmoDragState.startObjScale = (selectedObjectIndex == -1) ? glm::vec3(1.0f) : sceneObjects[selectedObjectIndex].scale;
            gizmoDragState.pivotPos = pivotPos;

            glm::vec3 rayOrig, rayDir;
            getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, rayOrig, rayDir);

            if (activeGizmo == GizmoType::TRANSLATE || activeGizmo == GizmoType::TRANSFORM_COMBINED || activeGizmo == GizmoType::SCALE || activeGizmo == GizmoType::RECT)
            {
                if (hoveredDragAxis == DragAxis::X)
                {
                    gizmoDragState.axisDir = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.startAxisVal = getClosestPointOnAxis(rayOrig, rayDir, pivotPos, gizmoDragState.axisDir, cameraWorldPos);
                }
                else if (hoveredDragAxis == DragAxis::Y)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.startAxisVal = getClosestPointOnAxis(rayOrig, rayDir, pivotPos, gizmoDragState.axisDir, cameraWorldPos);
                }
                else if (hoveredDragAxis == DragAxis::Z)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 0.0f, 1.0f);
                    gizmoDragState.startAxisVal = getClosestPointOnAxis(rayOrig, rayDir, pivotPos, gizmoDragState.axisDir, cameraWorldPos);
                }
                else if (hoveredDragAxis == DragAxis::XZ)
                {
                    gizmoDragState.planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                    intersectRayPlane(rayOrig, rayDir, pivotPos, glm::vec3(0.0f, 1.0f, 0.0f), gizmoDragState.startHitPoint);
                }
                else if (hoveredDragAxis == DragAxis::FREE)
                {
                    gizmoDragState.planeNormal = cameraForward;
                    intersectRayPlane(rayOrig, rayDir, pivotPos, gizmoDragState.planeNormal, gizmoDragState.startHitPoint);
                    gizmoDragState.startDist = glm::distance(gizmoDragState.startHitPoint, pivotPos);
                }
            }
            else if (activeGizmo == GizmoType::ROTATE)
            {
                if (hoveredDragAxis == DragAxis::X)
                {
                    gizmoDragState.axisDir = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.planeNormal = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.rotBasisU = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.rotBasisV = glm::vec3(0.0f, 0.0f, 1.0f);
                }
                else if (hoveredDragAxis == DragAxis::Y)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                    gizmoDragState.rotBasisU = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.rotBasisV = glm::vec3(0.0f, 0.0f, 1.0f);
                }
                else if (hoveredDragAxis == DragAxis::Z)
                {
                    gizmoDragState.axisDir = glm::vec3(0.0f, 0.0f, 1.0f);
                    gizmoDragState.planeNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                    gizmoDragState.rotBasisU = glm::vec3(1.0f, 0.0f, 0.0f);
                    gizmoDragState.rotBasisV = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                intersectRayPlane(rayOrig, rayDir, pivotPos, gizmoDragState.planeNormal, gizmoDragState.startHitPoint);
                glm::vec3 dirVec = gizmoDragState.startHitPoint - pivotPos;
                gizmoDragState.startAngle = atan2(glm::dot(dirVec, gizmoDragState.rotBasisV), glm::dot(dirVec, gizmoDragState.rotBasisU));
            }
        }
        else if (hoveredSceneObjIdx != -2)
        {
            selectedObjectIndex = hoveredSceneObjIdx;
            scene->setSelectedObjectIndex(selectedObjectIndex);
            hasSelection = true;
            pivotPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;

            if (activeGizmo != GizmoType::HAND)
            {
                scene->saveHistory();
                gizmoDragState.isDragging = true;
                gizmoDragState.axis = DragAxis::XZ;
                gizmoDragState.gizmoType = GizmoType::TRANSLATE;
                activeDragAxis = DragAxis::XZ;
                isDraggingObject = true;

                gizmoDragState.startObjPos = pivotPos;
                gizmoDragState.startObjRot = (selectedObjectIndex == -1) ? glm::vec3(0.0f) : sceneObjects[selectedObjectIndex].rotation;
                gizmoDragState.startObjScale = (selectedObjectIndex == -1) ? glm::vec3(1.0f) : sceneObjects[selectedObjectIndex].scale;
                gizmoDragState.pivotPos = pivotPos;
                gizmoDragState.planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);

                glm::vec3 rayOrig, rayDir;
                if (getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, rayOrig, rayDir))
                {
                    intersectRayPlane(rayOrig, rayDir, pivotPos, glm::vec3(0.0f, 1.0f, 0.0f), gizmoDragState.startHitPoint);
                }
            }
        }
        else
        {
            if (!ImGui::GetIO().KeyShift && !ImGui::GetIO().KeyCtrl)
            {
                selectedObjectIndex = -2;
                scene->setSelectedObjectIndex(-2);
            }
        }
    }

    // Camera Navigation Orbiting, Panning & Zooming
    if (isMouseInWindow && !isBlenderGrabMode)
    {
        // 1. Pan Camera (Shift + Middle Mouse or Shift + Right Mouse) like Blender / Maya / Unity
        if (ImGui::GetIO().KeyShift && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || ImGui::IsMouseDragging(ImGuiMouseButton_Right)))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ? ImGuiMouseButton_Middle : ImGuiMouseButton_Right);
            glm::vec3 cFwd = glm::normalize(sceneCameraTarget - sceneCamPos);
            glm::vec3 cRight = glm::normalize(glm::cross(cFwd, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 cUp = glm::normalize(glm::cross(cRight, cFwd));
            float panSpeed = sceneCameraDistance * 0.002f;
            sceneCameraTarget -= cRight * (delta.x * panSpeed);
            sceneCameraTarget += cUp * (delta.y * panSpeed);
            ImGui::ResetMouseDragDelta(ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ? ImGuiMouseButton_Middle : ImGuiMouseButton_Right);
        }
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
            sceneRotationY += delta.x * 0.4f;
            sceneRotationX += delta.y * 0.4f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        }
        else if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
            sceneRotationY += delta.x * 0.4f;
            sceneRotationX += delta.y * 0.4f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        }
        else if (ImGui::GetIO().KeyAlt && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            sceneRotationY += delta.x * 0.4f;
            sceneRotationX += delta.y * 0.4f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
        else if (activeGizmo == GizmoType::HAND && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            sceneRotationY += delta.x * 0.4f;
            sceneRotationX += delta.y * 0.4f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
        else if (!gizmoDragState.isDragging && !isDraggingObject && hoveredDragAxis == DragAxis::NONE && hoveredSceneObjIdx == -2 && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            sceneRotationY += delta.x * 0.4f;
            sceneRotationX += delta.y * 0.4f;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        sceneCameraDistance -= ImGui::GetIO().MouseWheel * 0.5f;
        sceneCameraDistance = std::clamp(sceneCameraDistance, 1.0f, 60.0f);
    }

    // 1. Draw Grid lines in XZ plane (y = -1.5)
    float gridY = -1.5f;
    for (int i = -5; i <= 5; ++i)
    {
        glm::vec3 p1((float)i, gridY, -5.0f);
        glm::vec3 p2((float)i, gridY, 5.0f);
        ImVec2 sp1 = projectPoint(p1, view, proj, windowPos, windowSize);
        ImVec2 sp2 = projectPoint(p2, view, proj, windowPos, windowSize);
        if (sp1.x > -90000.0f && sp2.x > -90000.0f)
        {
            ImU32 col = (i == 0) ? IM_COL32(0, 0, 180, 255) : IM_COL32(80, 80, 80, 255);
            drawList->AddLine(sp1, sp2, col, (i == 0) ? 2.0f : 1.0f);
        }

        glm::vec3 p3(-5.0f, gridY, (float)i);
        glm::vec3 p4(5.0f, gridY, (float)i);
        ImVec2 sp3 = projectPoint(p3, view, proj, windowPos, windowSize);
        ImVec2 sp4 = projectPoint(p4, view, proj, windowPos, windowSize);
        if (sp3.x > -90000.0f && sp4.x > -90000.0f)
        {
            ImU32 col = (i == 0) ? IM_COL32(180, 0, 0, 255) : IM_COL32(80, 80, 80, 255);
            drawList->AddLine(sp3, sp4, col, (i == 0) ? 2.0f : 1.0f);
        }
    }

    // 2. Draw 3D Origin Axes
    glm::vec3 orig(0.0f, gridY, 0.0f);
    glm::vec3 axX(1.0f, gridY, 0.0f);
    glm::vec3 axY(0.0f, gridY + 1.0f, 0.0f);
    glm::vec3 axZ(0.0f, gridY, 1.0f);
    ImVec2 sor = projectPoint(orig, view, proj, windowPos, windowSize);
    ImVec2 sx = projectPoint(axX, view, proj, windowPos, windowSize);
    ImVec2 sy = projectPoint(axY, view, proj, windowPos, windowSize);
    ImVec2 sz = projectPoint(axZ, view, proj, windowPos, windowSize);
    if (sor.x > -90000.0f)
    {
        if (sx.x > -90000.0f) {
            drawList->AddLine(sor, sx, IM_COL32(255, 0, 0, 255), 2.0f);
            drawList->AddText(ImVec2(sx.x + 4.0f, sx.y - 4.0f), IM_COL32(255, 100, 100, 255), "X");
        }
        if (sy.x > -90000.0f) {
            drawList->AddLine(sor, sy, IM_COL32(0, 255, 0, 255), 2.0f);
            drawList->AddText(ImVec2(sy.x + 4.0f, sy.y - 4.0f), IM_COL32(100, 255, 100, 255), "Y");
        }
        if (sz.x > -90000.0f) {
            drawList->AddLine(sor, sz, IM_COL32(0, 0, 255, 255), 2.0f);
            drawList->AddText(ImVec2(sz.x + 4.0f, sz.y - 4.0f), IM_COL32(100, 100, 255, 255), "Z");
        }
    }

    // 3. Draw Scene Objects in 3D Offscreen View Image
    ImGui::SetCursorScreenPos(windowPos);
    if (offscreenTexture) {
        ImGui::Image((ImTextureID)offscreenTexture, windowSize);
    }

    // 4. Draw Gizmos & Selection Highlights
    // 4.1 Hover Highlight (Cyan Outline & Floating Name Tag)
    if (hoveredSceneObjIdx >= 0 && hoveredSceneObjIdx != selectedObjectIndex && hoveredSceneObjIdx < static_cast<int>(sceneObjects.size()))
    {
        draw3DObject(*scene, hoveredSceneObjIdx, view, proj, windowPos, windowSize, IM_COL32(0, 230, 255, 220), 1.8f);

        glm::vec3 topWorld = sceneObjects[hoveredSceneObjIdx].position + glm::vec3(0.0f, std::max(0.6f, sceneObjects[hoveredSceneObjIdx].scale.y * 0.6f), 0.0f);
        ImVec2 sBadge = projectPoint(topWorld, view, proj, windowPos, windowSize);
        if (sBadge.x > -90000.0f && sBadge.x >= windowPos.x && sBadge.x <= windowPos.x + windowSize.x && sBadge.y >= windowPos.y && sBadge.y <= windowPos.y + windowSize.y)
        {
            std::string hoverText = "🔍 " + sceneObjects[hoveredSceneObjIdx].name;
            ImVec2 textSize = ImGui::CalcTextSize(hoverText.c_str());
            ImVec2 bMin(sBadge.x - textSize.x * 0.5f - 6.0f, sBadge.y - textSize.y - 6.0f);
            ImVec2 bMax(sBadge.x + textSize.x * 0.5f + 6.0f, sBadge.y - 2.0f);
            drawList->AddRectFilled(bMin, bMax, IM_COL32(15, 25, 35, 230), 4.0f);
            drawList->AddRect(bMin, bMax, IM_COL32(0, 230, 255, 255), 4.0f, 0, 1.2f);
            drawList->AddText(ImVec2(bMin.x + 6.0f, bMin.y + 2.0f), IM_COL32(230, 250, 255, 255), hoverText.c_str());
        }
    }

    // 4.2 Selection Highlight (Golden-Orange 3D Bounding Box, Ground Ring, and Name Tag)
    if (hasSelection && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        // 3D Bounding Box highlight
        draw3DObject(*scene, selectedObjectIndex, view, proj, windowPos, windowSize, IM_COL32(255, 185, 20, 255), 2.2f);

        const auto& selObj = sceneObjects[selectedObjectIndex];
        float selRadius = std::max({ std::abs(selObj.scale.x), std::abs(selObj.scale.z) }) * 0.8f;
        std::string lowerName = selObj.name;
        for (char& c : lowerName) c = (char)std::tolower((unsigned char)c);

        if (lowerName.find("tree") != std::string::npos) selRadius = std::max(1.0f, 1.4f * selObj.scale.x);
        else if (lowerName.find("house") != std::string::npos || lowerName.find("building") != std::string::npos) selRadius = std::max(1.5f, 2.2f * selObj.scale.x);
        else if (lowerName.find("rock") != std::string::npos) selRadius = std::max(0.8f, 1.2f * selObj.scale.x);
        selRadius = std::max(0.6f, selRadius);

        const int ringSegs = 32;
        ImVec2 prevPt(-99999.0f, -99999.0f);
        for (int s = 0; s <= ringSegs; ++s)
        {
            float ang = (s % ringSegs) * 2.0f * 3.14159f / ringSegs;
            glm::vec3 rPt = glm::vec3(selObj.position.x + std::cos(ang) * selRadius, selObj.position.y + 0.05f, selObj.position.z + std::sin(ang) * selRadius);
            ImVec2 sPt = projectPoint(rPt, view, proj, windowPos, windowSize);
            if (s > 0 && prevPt.x > -90000.0f && sPt.x > -90000.0f)
            {
                drawList->AddLine(prevPt, sPt, IM_COL32(255, 200, 40, 220), 2.0f);
            }
            prevPt = sPt;
        }

        // Floating Selection Name Tag
        glm::vec3 topWorld = selObj.position + glm::vec3(0.0f, std::max(0.7f, selObj.scale.y * 0.75f), 0.0f);
        ImVec2 sBadge = projectPoint(topWorld, view, proj, windowPos, windowSize);
        if (sBadge.x > -90000.0f && sBadge.x >= windowPos.x && sBadge.x <= windowPos.x + windowSize.x && sBadge.y >= windowPos.y && sBadge.y <= windowPos.y + windowSize.y)
        {
            std::string selText = "🎯 " + selObj.name + " [F: Focus]";
            ImVec2 textSize = ImGui::CalcTextSize(selText.c_str());
            ImVec2 bMin(sBadge.x - textSize.x * 0.5f - 6.0f, sBadge.y - textSize.y - 6.0f);
            ImVec2 bMax(sBadge.x + textSize.x * 0.5f + 6.0f, sBadge.y - 2.0f);
            drawList->AddRectFilled(bMin, bMax, IM_COL32(35, 25, 10, 235), 4.0f);
            drawList->AddRect(bMin, bMax, IM_COL32(255, 185, 20, 255), 4.0f, 0, 1.5f);
            drawList->AddText(ImVec2(bMin.x + 6.0f, bMin.y + 2.0f), IM_COL32(255, 220, 100, 255), selText.c_str());
        }
    }

    // Blender Grab Mode Guideline & Controls
    if (isBlenderGrabMode && hasSelection)
    {
        glm::vec3 objPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
        if (grabConstrainAxis == 0)
        {
            glm::vec3 pA = objPos - glm::vec3(50.0f, 0.0f, 0.0f);
            glm::vec3 pB = objPos + glm::vec3(50.0f, 0.0f, 0.0f);
            ImVec2 spA = projectPoint(pA, view, proj, windowPos, windowSize);
            ImVec2 spB = projectPoint(pB, view, proj, windowPos, windowSize);
            if (spA.x > -90000.0f && spB.x > -90000.0f)
                drawList->AddLine(spA, spB, IM_COL32(255, 60, 60, 220), 2.0f);
        }
        else if (grabConstrainAxis == 1)
        {
            glm::vec3 pA = objPos - glm::vec3(0.0f, 50.0f, 0.0f);
            glm::vec3 pB = objPos + glm::vec3(0.0f, 50.0f, 0.0f);
            ImVec2 spA = projectPoint(pA, view, proj, windowPos, windowSize);
            ImVec2 spB = projectPoint(pB, view, proj, windowPos, windowSize);
            if (spA.x > -90000.0f && spB.x > -90000.0f)
                drawList->AddLine(spA, spB, IM_COL32(60, 255, 60, 220), 2.0f);
        }
        else if (grabConstrainAxis == 2)
        {
            glm::vec3 pA = objPos - glm::vec3(0.0f, 0.0f, 50.0f);
            glm::vec3 pB = objPos + glm::vec3(0.0f, 0.0f, 50.0f);
            ImVec2 spA = projectPoint(pA, view, proj, windowPos, windowSize);
            ImVec2 spB = projectPoint(pB, view, proj, windowPos, windowSize);
            if (spA.x > -90000.0f && spB.x > -90000.0f)
                drawList->AddLine(spA, spB, IM_COL32(60, 100, 255, 220), 2.0f);
        }
        else
        {
            ImVec2 spStart = projectPoint(grabStartPos, view, proj, windowPos, windowSize);
            ImVec2 spCur = projectPoint(objPos, view, proj, windowPos, windowSize);
            if (spStart.x > -90000.0f && spCur.x > -90000.0f)
                drawList->AddLine(spStart, spCur, IM_COL32(255, 220, 50, 180), 1.5f);
        }

        float barW = 560.0f;
        float barH = 34.0f;
        ImVec2 barPos(windowPos.x + (windowSize.x - barW) * 0.5f, windowPos.y + windowSize.y - barH - 15.0f);
        drawList->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(18, 22, 30, 240), 6.0f);
        drawList->AddRect(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(255, 180, 0, 255), 6.0f, 0, 2.0f);

        const char* axisLockStr = (grabConstrainAxis == 0) ? "X-Axis [Red]" : (grabConstrainAxis == 1) ? "Y-Axis [Green]" : (grabConstrainAxis == 2) ? "Z-Axis [Blue]" : "Free Ground (XZ)";
        char grabMsg[256];
        snprintf(grabMsg, sizeof(grabMsg), "GRAB (G): Move mouse | Axis: %s | [X/Y/Z] Lock | [L-Click/Enter] Confirm | [R-Click/Esc] Cancel", axisLockStr);
        drawList->AddText(ImVec2(barPos.x + 14.0f, barPos.y + 8.0f), IM_COL32(255, 255, 255, 255), grabMsg);
    }

    DragAxis activeHighlight = gizmoDragState.isDragging ? gizmoDragState.axis : hoveredDragAxis;
    if (hasSelection && sP.x > -90000.0f && activeGizmo != GizmoType::HAND)
    {
        ImU32 centerCol = (activeHighlight == DragAxis::FREE) ? IM_COL32(255, 255, 100, 255) : IM_COL32(255, 255, 0, 255);
        drawList->AddCircle(sP, (activeHighlight == DragAxis::FREE) ? 13.0f : 11.0f, centerCol, 0, 2.0f);
        drawList->AddLine(ImVec2(sP.x - 7, sP.y), ImVec2(sP.x + 7, sP.y), centerCol, 1.5f);
        drawList->AddLine(ImVec2(sP.x, sP.y - 7), ImVec2(sP.x, sP.y + 7), centerCol, 1.5f);

        if (activeGizmo == GizmoType::TRANSLATE || activeGizmo == GizmoType::TRANSFORM_COMBINED)
        {
            if (sPlaneX.x > -90000.0f && sPlaneZ.x > -90000.0f && sXZ.x > -90000.0f && sP.x > -90000.0f)
            {
                bool isH = (activeHighlight == DragAxis::XZ);
                ImU32 fillCol = isH ? IM_COL32(0, 230, 230, 180) : IM_COL32(0, 180, 180, 90);
                ImU32 borderCol = isH ? IM_COL32(0, 255, 255, 255) : IM_COL32(0, 210, 210, 200);
                drawList->AddQuadFilled(sP, sPlaneX, sXZ, sPlaneZ, fillCol);
                drawList->AddQuad(sP, sPlaneX, sXZ, sPlaneZ, borderCol, 1.5f);
            }

            if (sX.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::X) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::X) ? IM_COL32(255, 130, 130, 255) : IM_COL32(255, 40, 40, 255);
                drawList->AddLine(sP, sX, col, thick);
                drawList->AddRectFilled(ImVec2(sX.x - 5, sX.y - 5), ImVec2(sX.x + 5, sX.y + 5), col);
            }

            if (sY.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Y) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Y) ? IM_COL32(130, 255, 130, 255) : IM_COL32(40, 255, 40, 255);
                drawList->AddLine(sP, sY, col, thick);
                drawList->AddRectFilled(ImVec2(sY.x - 5, sY.y - 5), ImVec2(sY.x + 5, sY.y + 5), col);
            }

            if (sZ.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Z) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Z) ? IM_COL32(130, 130, 255, 255) : IM_COL32(40, 40, 255, 255);
                drawList->AddLine(sP, sZ, col, thick);
                drawList->AddRectFilled(ImVec2(sZ.x - 5, sZ.y - 5), ImVec2(sZ.x + 5, sZ.y + 5), col);
            }
        }

        if (activeGizmo == GizmoType::ROTATE || activeGizmo == GizmoType::TRANSFORM_COMBINED)
        {
            const int numSegs = 36;
            ImVec2 prevX, prevY, prevZ;
            for (int i = 0; i <= numSegs; ++i)
            {
                float a = (i * 2.0f * 3.14159f) / numSegs;

                glm::vec3 ptX = pivotPos + glm::vec3(0.0f, cos(a) * L, sin(a) * L);
                ImVec2 sPtX = projectPoint(ptX, view, proj, windowPos, windowSize);
                if (i > 0 && prevX.x > -90000.0f && sPtX.x > -90000.0f)
                {
                    bool isH = (activeHighlight == DragAxis::X);
                    ImU32 col = isH ? IM_COL32(255, 140, 140, 255) : IM_COL32(255, 60, 60, 200);
                    drawList->AddLine(prevX, sPtX, col, isH ? 3.5f : 1.8f);
                }
                prevX = sPtX;

                glm::vec3 ptY = pivotPos + glm::vec3(cos(a) * L, 0.0f, sin(a) * L);
                ImVec2 sPtY = projectPoint(ptY, view, proj, windowPos, windowSize);
                if (i > 0 && prevY.x > -90000.0f && sPtY.x > -90000.0f)
                {
                    bool isH = (activeHighlight == DragAxis::Y);
                    ImU32 col = isH ? IM_COL32(140, 255, 140, 255) : IM_COL32(60, 255, 60, 200);
                    drawList->AddLine(prevY, sPtY, col, isH ? 3.5f : 1.8f);
                }
                prevY = sPtY;

                glm::vec3 ptZ = pivotPos + glm::vec3(cos(a) * L, sin(a) * L, 0.0f);
                ImVec2 sPtZ = projectPoint(ptZ, view, proj, windowPos, windowSize);
                if (i > 0 && prevZ.x > -90000.0f && sPtZ.x > -90000.0f)
                {
                    bool isH = (activeHighlight == DragAxis::Z);
                    ImU32 col = isH ? IM_COL32(140, 140, 255, 255) : IM_COL32(60, 60, 255, 200);
                    drawList->AddLine(prevZ, sPtZ, col, isH ? 3.5f : 1.8f);
                }
                prevZ = sPtZ;
            }

            if (activeGizmo == GizmoType::ROTATE)
            {
                if (sX.x > -90000.0f) drawList->AddCircleFilled(sX, 5.0f, (activeHighlight == DragAxis::X) ? IM_COL32(255, 160, 160, 255) : IM_COL32(255, 0, 0, 255));
                if (sY.x > -90000.0f) drawList->AddCircleFilled(sY, 5.0f, (activeHighlight == DragAxis::Y) ? IM_COL32(160, 255, 160, 255) : IM_COL32(0, 255, 0, 255));
                if (sZ.x > -90000.0f) drawList->AddCircleFilled(sZ, 5.0f, (activeHighlight == DragAxis::Z) ? IM_COL32(160, 160, 255, 255) : IM_COL32(0, 0, 255, 255));
            }
        }

        if (activeGizmo == GizmoType::SCALE)
        {
            if (sX.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::X) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::X) ? IM_COL32(255, 130, 130, 255) : IM_COL32(255, 40, 40, 255);
                drawList->AddLine(sP, sX, col, thick);
                drawList->AddRectFilled(ImVec2(sX.x - 5, sX.y - 5), ImVec2(sX.x + 5, sX.y + 5), col);
            }
            if (sY.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Y) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Y) ? IM_COL32(130, 255, 130, 255) : IM_COL32(40, 255, 40, 255);
                drawList->AddLine(sP, sY, col, thick);
                drawList->AddRectFilled(ImVec2(sY.x - 5, sY.y - 5), ImVec2(sY.x + 5, sY.y + 5), col);
            }
            if (sZ.x > -90000.0f)
            {
                float thick = (activeHighlight == DragAxis::Z) ? 3.5f : 1.8f;
                ImU32 col = (activeHighlight == DragAxis::Z) ? IM_COL32(130, 130, 255, 255) : IM_COL32(40, 40, 255, 255);
                drawList->AddLine(sP, sZ, col, thick);
                drawList->AddRectFilled(ImVec2(sZ.x - 5, sZ.y - 5), ImVec2(sZ.x + 5, sZ.y + 5), col);
            }
        }

        if (activeGizmo == GizmoType::RECT)
        {
            glm::vec3 halfX(L * 0.8f, 0.0f, 0.0f);
            glm::vec3 halfZ(0.0f, 0.0f, L * 0.8f);

            ImVec2 tl = projectPoint(pivotPos - halfX + halfZ, view, proj, windowPos, windowSize);
            ImVec2 tr = projectPoint(pivotPos + halfX + halfZ, view, proj, windowPos, windowSize);
            ImVec2 br = projectPoint(pivotPos + halfX - halfZ, view, proj, windowPos, windowSize);
            ImVec2 bl = projectPoint(pivotPos - halfX - halfZ, view, proj, windowPos, windowSize);

            if (tl.x > -90000.0f && tr.x > -90000.0f && br.x > -90000.0f && bl.x > -90000.0f)
            {
                ImU32 rectColor = IM_COL32(200, 200, 200, 150);
                drawList->AddQuad(tl, tr, br, bl, rectColor, 1.5f);

                if (sX.x > -90000.0f) drawList->AddCircleFilled(sX, 4.0f, IM_COL32(255, 0, 0, 255));
                if (sZ.x > -90000.0f) drawList->AddCircleFilled(sZ, 4.0f, IM_COL32(0, 0, 255, 255));
            }
        }
    }

    // 5. Floating Active Drag Tooltip & Status Badge
    if (gizmoDragState.isDragging && hasSelection)
    {
        std::string objName = (selectedObjectIndex == -1) ? "Main Camera" : sceneObjects[selectedObjectIndex].name;
        const char* axisNames[] = { "NONE", "X Axis", "Y Axis", "Z Axis", "XY Plane", "YZ Plane", "XZ Plane", "Free" };
        const char* axisStr = axisNames[static_cast<int>(gizmoDragState.axis)];

        ImU32 axisBadgeCol = IM_COL32(255, 255, 100, 255);
        if (gizmoDragState.axis == DragAxis::X) axisBadgeCol = IM_COL32(255, 80, 80, 255);
        else if (gizmoDragState.axis == DragAxis::Y) axisBadgeCol = IM_COL32(80, 255, 80, 255);
        else if (gizmoDragState.axis == DragAxis::Z) axisBadgeCol = IM_COL32(80, 150, 255, 255);
        else if (gizmoDragState.axis == DragAxis::XZ) axisBadgeCol = IM_COL32(0, 220, 220, 255);

        glm::vec3 curPos = (selectedObjectIndex == -1) ? mainCameraPos : sceneObjects[selectedObjectIndex].position;
        glm::vec3 curRot = (selectedObjectIndex == -1) ? glm::vec3(0.0f) : sceneObjects[selectedObjectIndex].rotation;
        glm::vec3 curScale = (selectedObjectIndex == -1) ? glm::vec3(1.0f) : sceneObjects[selectedObjectIndex].scale;

        char badgeLine1[128];
        char badgeLine2[128];
        char badgeLine3[128] = "";

        if (gizmoDragState.gizmoType == GizmoType::TRANSLATE || gizmoDragState.gizmoType == GizmoType::TRANSFORM_COMBINED)
        {
            snprintf(badgeLine1, sizeof(badgeLine1), "Moving: %s [%s]", objName.c_str(), axisStr);
            if (gizmoDragState.axis == DragAxis::X)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos X: %.2fm  (DX: %+.2fm)", curPos.x, curPos.x - gizmoDragState.startObjPos.x);
            else if (gizmoDragState.axis == DragAxis::Y)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos Y: %.2fm  (DY: %+.2fm)", curPos.y, curPos.y - gizmoDragState.startObjPos.y);
            else if (gizmoDragState.axis == DragAxis::Z)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos Z: %.2fm  (DZ: %+.2fm)", curPos.z, curPos.z - gizmoDragState.startObjPos.z);
            else if (gizmoDragState.axis == DragAxis::XZ)
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos XZ: (%.2f, %.2f)  (D: %+.2f, %+.2f)", 
                    curPos.x, curPos.z, curPos.x - gizmoDragState.startObjPos.x, curPos.z - gizmoDragState.startObjPos.z);
            else
                snprintf(badgeLine2, sizeof(badgeLine2), "Pos: (%.2f, %.2f, %.2f)", curPos.x, curPos.y, curPos.z);
        }
        else if (gizmoDragState.gizmoType == GizmoType::ROTATE)
        {
            snprintf(badgeLine1, sizeof(badgeLine1), "Rotating: %s [%s]", objName.c_str(), axisStr);
            if (gizmoDragState.axis == DragAxis::X)
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot X: %.1f deg  (D: %+.1f deg)", curRot.x, curRot.x - gizmoDragState.startObjRot.x);
            else if (gizmoDragState.axis == DragAxis::Y)
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot Y: %.1f deg  (D: %+.1f deg)", curRot.y, curRot.y - gizmoDragState.startObjRot.y);
            else if (gizmoDragState.axis == DragAxis::Z)
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot Z: %.1f deg  (D: %+.1f deg)", curRot.z, curRot.z - gizmoDragState.startObjRot.z);
            else
                snprintf(badgeLine2, sizeof(badgeLine2), "Rot: (%.1f, %.1f, %.1f)", curRot.x, curRot.y, curRot.z);
        }
        else if (gizmoDragState.gizmoType == GizmoType::SCALE || gizmoDragState.gizmoType == GizmoType::RECT)
        {
            snprintf(badgeLine1, sizeof(badgeLine1), "Scaling: %s [%s]", objName.c_str(), axisStr);
            if (gizmoDragState.axis == DragAxis::X)
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale X: %.2f  (Ratio: %.2fx)", curScale.x, (gizmoDragState.startObjScale.x > 0.001f) ? curScale.x / gizmoDragState.startObjScale.x : 1.0f);
            else if (gizmoDragState.axis == DragAxis::Y)
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale Y: %.2f  (Ratio: %.2fx)", curScale.y, (gizmoDragState.startObjScale.y > 0.001f) ? curScale.y / gizmoDragState.startObjScale.y : 1.0f);
            else if (gizmoDragState.axis == DragAxis::Z)
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale Z: %.2f  (Ratio: %.2fx)", curScale.z, (gizmoDragState.startObjScale.z > 0.001f) ? curScale.z / gizmoDragState.startObjScale.z : 1.0f);
            else
                snprintf(badgeLine2, sizeof(badgeLine2), "Scale: (%.2f, %.2f, %.2f)", curScale.x, curScale.y, curScale.z);
        }

        if (ImGui::GetIO().KeyCtrl)
        {
            snprintf(badgeLine3, sizeof(badgeLine3), "SNAP GRID ACTIVE");
        }

        ImVec2 badgePos = ImVec2(mousePos.x + 20.0f, mousePos.y + 15.0f);
        float badgeWidth = 250.0f;
        float badgeHeight = (badgeLine3[0] != '\0') ? 66.0f : 48.0f;

        if (badgePos.x + badgeWidth > windowPos.x + windowSize.x - 10.0f)
            badgePos.x = mousePos.x - badgeWidth - 10.0f;
        if (badgePos.y + badgeHeight > windowPos.y + windowSize.y - 10.0f)
            badgePos.y = mousePos.y - badgeHeight - 10.0f;

        drawList->AddRectFilled(badgePos, ImVec2(badgePos.x + badgeWidth, badgePos.y + badgeHeight), IM_COL32(15, 18, 24, 235), 6.0f);
        drawList->AddRect(badgePos, ImVec2(badgePos.x + badgeWidth, badgePos.y + badgeHeight), axisBadgeCol, 6.0f, 0, 1.8f);

        drawList->AddText(ImVec2(badgePos.x + 10.0f, badgePos.y + 6.0f), IM_COL32(255, 255, 255, 255), badgeLine1);
        drawList->AddText(ImVec2(badgePos.x + 10.0f, badgePos.y + 24.0f), IM_COL32(200, 220, 255, 255), badgeLine2);
        if (badgeLine3[0] != '\0')
        {
            drawList->AddText(ImVec2(badgePos.x + 10.0f, badgePos.y + 44.0f), IM_COL32(100, 255, 255, 255), badgeLine3);
        }
    }

    // 5. Draw 3D Camera Gizmo and frustum
    glm::vec3 camPos = mainCameraPos;
    glm::vec3 camTarget = mainCameraTarget;
    glm::vec3 forwardVector = glm::normalize(camTarget - camPos);
    glm::vec3 rightVector = glm::cross(forwardVector, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(rightVector) < 0.01f) rightVector = glm::vec3(1.0f, 0.0f, 0.0f);
    else rightVector = glm::normalize(rightVector);
    glm::vec3 upVector = glm::cross(rightVector, forwardVector);

    float frustumDist = 0.5f;
    float frustumW = 0.25f;
    float frustumH = 0.18f;

    glm::vec3 cameraBaseCenter = camPos + forwardVector * frustumDist;
    glm::vec3 c0 = cameraBaseCenter - rightVector * frustumW + upVector * frustumH;
    glm::vec3 c1 = cameraBaseCenter + rightVector * frustumW + upVector * frustumH;
    glm::vec3 c2 = cameraBaseCenter + rightVector * frustumW - upVector * frustumH;
    glm::vec3 c3 = cameraBaseCenter - rightVector * frustumW - upVector * frustumH;

    ImVec2 scamPos = projectPoint(camPos, view, proj, windowPos, windowSize);
    ImVec2 sc0 = projectPoint(c0, view, proj, windowPos, windowSize);
    ImVec2 sc1 = projectPoint(c1, view, proj, windowPos, windowSize);
    ImVec2 sc2 = projectPoint(c2, view, proj, windowPos, windowSize);
    ImVec2 sc3 = projectPoint(c3, view, proj, windowPos, windowSize);

    ImU32 frustumColor = IM_COL32(0, 240, 255, 255);
    
    if (scamPos.x > -90000.0f)
    {
        if (sc0.x > -90000.0f) drawList->AddLine(scamPos, sc0, frustumColor, 1.5f);
        if (sc1.x > -90000.0f) drawList->AddLine(scamPos, sc1, frustumColor, 1.5f);
        if (sc2.x > -90000.0f) drawList->AddLine(scamPos, sc2, frustumColor, 1.5f);
        if (sc3.x > -90000.0f) drawList->AddLine(scamPos, sc3, frustumColor, 1.5f);

        if (sc0.x > -90000.0f && sc1.x > -90000.0f) drawList->AddLine(sc0, sc1, frustumColor, 1.5f);
        if (sc1.x > -90000.0f && sc2.x > -90000.0f) drawList->AddLine(sc1, sc2, frustumColor, 1.5f);
        if (sc2.x > -90000.0f && sc3.x > -90000.0f) drawList->AddLine(sc2, sc3, frustumColor, 1.5f);
        if (sc3.x > -90000.0f && sc0.x > -90000.0f) drawList->AddLine(sc3, sc0, frustumColor, 1.5f);

        glm::vec3 bodyBack = camPos - forwardVector * 0.15f;
        glm::vec3 b0 = bodyBack - rightVector * 0.12f + upVector * 0.09f;
        glm::vec3 b1 = bodyBack + rightVector * 0.12f + upVector * 0.09f;
        glm::vec3 b2 = bodyBack + rightVector * 0.12f - upVector * 0.09f;
        glm::vec3 b3 = bodyBack - rightVector * 0.12f - upVector * 0.09f;

        ImVec2 sb0 = projectPoint(b0, view, proj, windowPos, windowSize);
        ImVec2 sb1 = projectPoint(b1, view, proj, windowPos, windowSize);
        ImVec2 sb2 = projectPoint(b2, view, proj, windowPos, windowSize);
        ImVec2 sb3 = projectPoint(b3, view, proj, windowPos, windowSize);

        if (sb0.x > -90000.0f && sb1.x > -90000.0f && sb2.x > -90000.0f && sb3.x > -90000.0f)
        {
            drawList->AddLine(sb0, sb1, frustumColor, 1.0f);
            drawList->AddLine(sb1, sb2, frustumColor, 1.0f);
            drawList->AddLine(sb2, sb3, frustumColor, 1.0f);
            drawList->AddLine(sb3, sb0, frustumColor, 1.0f);

            drawList->AddLine(scamPos, sb0, frustumColor, 1.0f);
            drawList->AddLine(scamPos, sb1, frustumColor, 1.0f);
            drawList->AddLine(scamPos, sb2, frustumColor, 1.0f);
            drawList->AddLine(scamPos, sb3, frustumColor, 1.0f);
        }

        drawList->AddCircleFilled(scamPos, 7.0f, IM_COL32(0, 240, 255, 255));
        drawList->AddCircle(scamPos, 11.0f, IM_COL32(0, 240, 255, 120), 0, 1.5f);
        drawList->AddText(ImVec2(scamPos.x + 12.0f, scamPos.y - 7.0f), IM_COL32(0, 240, 255, 255), "Main Camera");

        ImVec2 sorPoint = projectPoint(camTarget, view, proj, windowPos, windowSize);
        if (sorPoint.x > -90000.0f)
        {
            drawList->AddLine(scamPos, sorPoint, IM_COL32(0, 240, 255, 100), 1.0f);
        }
    }

    // 6. Drag & Drop Target over Scene View
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payloadModel = ImGui::AcceptDragDropPayload("DND_ASSET_MODEL");
        const ImGuiPayload* payloadTex = ImGui::AcceptDragDropPayload("DND_ASSET_TEXTURE");
        const ImGuiPayload* payloadLua = ImGui::AcceptDragDropPayload("DND_ASSET_LUA");
        const ImGuiPayload* payloadGeneric = ImGui::AcceptDragDropPayload("DND_ASSET_PATH");

        const ImGuiPayload* payload = payloadModel ? payloadModel : (payloadTex ? payloadTex : (payloadLua ? payloadLua : payloadGeneric));

        if (payload && assetManager)
        {
            const char* assetPath = static_cast<const char*>(payload->Data);
            std::string pathStr(assetPath);

            glm::vec3 rayOrig, rayDir;
            glm::vec3 dropPos(0.0f, 0.0f, 0.0f);
            if (getRayFromScreenPos(mousePos, windowPos, windowSize, view, proj, rayOrig, rayDir))
            {
                glm::vec3 hitPoint;
                if (intersectRayPlane(rayOrig, rayDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), hitPoint))
                {
                    dropPos = hitPoint;
                }
            }

            std::filesystem::path p(pathStr);
            std::string ext = p.extension().string();
            for (auto& c : ext) c = (char)tolower(c);

            scene->saveHistory();

            if (pathStr == "PRIMITIVE_CUBE")
            {
                SceneObject newObj;
                newObj.id = (int)sceneObjects.size();
                newObj.name = "Cube " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::CUBE;
                newObj.position = dropPos;
                newObj.scale = glm::vec3(0.5f);
                newObj.color = glm::vec4(1.0f);
                newObj.meshId = assetManager->getCubeMeshId();
                newObj.syncComponents();
                sceneObjects.push_back(newObj);
                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);
            }
            else if (pathStr == "PRIMITIVE_SPHERE")
            {
                SceneObject newObj;
                newObj.id = (int)sceneObjects.size();
                newObj.name = "Sphere " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::SPHERE;
                newObj.position = dropPos;
                newObj.scale = glm::vec3(0.5f);
                newObj.color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);
                newObj.meshId = assetManager->getSphereMeshId();
                newObj.syncComponents();
                sceneObjects.push_back(newObj);
                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);
            }
            else if (pathStr == "PRIMITIVE_PLANE")
            {
                SceneObject newObj;
                newObj.id = (int)sceneObjects.size();
                newObj.name = "Plane " + std::to_string(sceneObjects.size());
                newObj.type = ObjectType::PLANE;
                newObj.position = dropPos;
                newObj.scale = glm::vec3(2.0f, 1.0f, 2.0f);
                newObj.color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                newObj.meshId = assetManager->getPlaneMeshId();
                newObj.syncComponents();
                sceneObjects.push_back(newObj);
                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);
            }
            else if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
            {
                int meshId = assetManager->load3DModelAsset(pathStr);
                if (meshId >= 0)
                {
                    SceneObject newObj;
                    newObj.id = (int)sceneObjects.size();
                    std::string sName = p.stem().string();
                    newObj.name = sName;
                    newObj.type = ObjectType::CUBE;
                    newObj.position = dropPos;
                    float sc = 1.0f;
                    if (sName == "Fox") sc = 0.02f;
                    else if (sName == "Lantern") sc = 0.2f;

                    if (sName.find("rock") != std::string::npos || sName.find("Rock") != std::string::npos)
                    {
                        newObj.scale = glm::vec3(0.35f, 0.10f, 0.35f);
                        newObj.rotation = glm::vec3(0.0f);
                    }
                    else
                    {
                        newObj.scale = glm::vec3(sc);
                    }
                    newObj.color = glm::vec4(1.0f);
                    newObj.meshId = meshId;
                    const auto& meshes = assetManager->getMeshes();
                    if (meshId < static_cast<int>(meshes.size())) {
                        newObj.textureId = meshes[meshId].defaultTextureId;
                        newObj.roughness = meshes[meshId].defaultRoughness;
                        newObj.metallic = meshes[meshId].defaultMetallic;
                    }
                    newObj.syncComponents();
                    sceneObjects.push_back(newObj);
                    scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);
                }
            }
            else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
            {
                int texId = assetManager->loadTextureAsset(pathStr);
                if (texId >= 0 && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    sceneObjects[selectedObjectIndex].textureId = texId;
                    sceneObjects[selectedObjectIndex].syncComponents();
                }
            }
            else if (ext == ".lua")
            {
                if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                {
                    std::ifstream t(pathStr);
                    if (t.is_open())
                    {
                        std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                        sceneObjects[selectedObjectIndex].luaScripts.push_back(scriptContent);
                        sceneObjects[selectedObjectIndex].syncComponents();
                    }
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void GizmoSystem::drawGameView(VkDescriptorSet gameViewTexture, const ImVec2& windowPos, const ImVec2& windowSize,
                               int gameScore, int highScore, std::function<void()> onToggleFullscreen, AppMode mode)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // 1. Draw Space Gradient Background
    drawList->AddRectFilledMultiColor(
        windowPos, 
        ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
        IM_COL32(6, 10, 20, 255),
        IM_COL32(8, 12, 24, 255),
        IM_COL32(16, 26, 46, 255),
        IM_COL32(10, 18, 32, 255)
    );

    // 2. Draw Planetary Nebula Glow
    drawList->AddCircleFilled(ImVec2(windowPos.x + windowSize.x * 0.75f, windowPos.y + windowSize.y * 0.70f), 130.0f, IM_COL32(0, 120, 255, 12), 64);
    drawList->AddCircleFilled(ImVec2(windowPos.x + windowSize.x * 0.75f, windowPos.y + windowSize.y * 0.70f), 80.0f, IM_COL32(100, 180, 255, 18), 64);
    drawList->AddCircleFilled(ImVec2(windowPos.x + windowSize.x * 0.25f, windowPos.y + windowSize.y * 0.30f), 200.0f, IM_COL32(120, 80, 255, 8), 64);

    // 3. Draw Stars
    for (int i = 0; i < 40; ++i)
    {
        float sx = static_cast<float>((i * 13579) % static_cast<int>(windowSize.x > 20.0f ? windowSize.x - 20 : 1) + 10);
        float sy = static_cast<float>((i * 24680) % static_cast<int>(windowSize.y > 20.0f ? windowSize.y - 20 : 1) + 10);
        float radius = (i % 4 == 0) ? 1.5f : (i % 7 == 0 ? 2.0f : 1.0f);
        ImU32 starCol = (i % 6 == 0) ? IM_COL32(200, 230, 255, 230) : IM_COL32(255, 255, 255, 160);
        if (i % 12 == 0) {
            drawList->AddLine(ImVec2(windowPos.x + sx - 3, windowPos.y + sy), ImVec2(windowPos.x + sx + 3, windowPos.y + sy), IM_COL32(255, 255, 255, 150));
            drawList->AddLine(ImVec2(windowPos.x + sx, windowPos.y + sy - 3), ImVec2(windowPos.x + sx, windowPos.y + sy + 3), IM_COL32(255, 255, 255, 150));
        }
        drawList->AddCircleFilled(ImVec2(windowPos.x + sx, windowPos.y + sy), radius, starCol);
    }

    // Camera Matrices for Game view
    glm::mat4 gameProj = glm::perspective(glm::radians(mainCameraFov), windowSize.x / (windowSize.y > 0.0f ? windowSize.y : 1.0f), mainCameraNear, mainCameraFar);
    glm::mat4 gameView = glm::lookAt(mainCameraPos, mainCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    // 4. Draw Ground Platform in Game View
    float gridY = -1.5f;
    for (int i = -4; i <= 4; ++i)
    {
        glm::vec3 p1((float)i, gridY, -4.0f);
        glm::vec3 p2((float)i, gridY, 4.0f);
        ImVec2 sp1 = projectPoint(p1, gameView, gameProj, windowPos, windowSize);
        ImVec2 sp2 = projectPoint(p2, gameView, gameProj, windowPos, windowSize);
        if (sp1.x > -90000.0f && sp2.x > -90000.0f)
            drawList->AddLine(sp1, sp2, IM_COL32(50, 80, 100, 180), 1.0f);

        glm::vec3 p3(-4.0f, gridY, (float)i);
        glm::vec3 p4(4.0f, gridY, (float)i);
        ImVec2 sp3 = projectPoint(p3, gameView, gameProj, windowPos, windowSize);
        ImVec2 sp4 = projectPoint(p4, gameView, gameProj, windowPos, windowSize);
        if (sp3.x > -90000.0f && sp4.x > -90000.0f)
            drawList->AddLine(sp3, sp4, IM_COL32(50, 80, 100, 180), 1.0f);
    }

    // 5. Draw 3D offscreen game view texture
    ImGui::SetCursorScreenPos(windowPos);
    if (gameViewTexture) {
        ImGui::Image((ImTextureID)gameViewTexture, windowSize);
    }

    // 6. Draw Game HUD (Score Overlay)
    drawList->AddRectFilled(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + 220, windowPos.y + 85), IM_COL32(15, 20, 30, 200), 4.0f);
    drawList->AddRect(ImVec2(windowPos.x + 10, windowPos.y + 10), ImVec2(windowPos.x + 220, windowPos.y + 85), IM_COL32(0, 180, 255, 150), 4.0f);

    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 15), IM_COL32(0, 240, 255, 255), "ANTIGRAVITY mini-game");
    char scoreText[64];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", gameScore);
    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 35), IM_COL32(50, 255, 100, 255), scoreText);
    char hiText[64];
    snprintf(hiText, sizeof(hiText), "High Score: %d", highScore);
    drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 50), IM_COL32(255, 215, 0, 255), hiText);
    
    if (mode == AppMode::PLAY)
    {
        drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 65), IM_COL32(200, 200, 200, 255), "Keys: WASD + Space (Jump)");
    }
    else
    {
        drawList->AddText(ImVec2(windowPos.x + 20, windowPos.y + 65), IM_COL32(255, 150, 0, 255), "EDIT MODE: Click PLAY above");
    }

    // Fullscreen Toggle Button in Top-Right corner of Game View
    ImGui::SetCursorScreenPos(ImVec2(windowPos.x + windowSize.x - 205.0f, windowPos.y + 10.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, isGameFullscreen ? ImVec4(0.7f, 0.2f, 0.2f, 0.9f) : ImVec4(0.12f, 0.45f, 0.75f, 0.9f));
    std::string fsBtnLabel = isGameFullscreen ? "Exit Fullscreen (ESC)" : "Fullscreen (Monitor 2)";
    if (ImGui::Button(fsBtnLabel.c_str(), ImVec2(195, 30)))
    {
        if (onToggleFullscreen) onToggleFullscreen();
    }
    ImGui::PopStyleColor();
}

void GizmoSystem::focusOnObject(int objIndex, const Scene& scene)
{
    const auto& objects = scene.getObjects();
    if (objIndex >= 0 && objIndex < static_cast<int>(objects.size()))
    {
        const auto& obj = objects[objIndex];
        sceneCameraTarget = obj.position;
        float maxDim = std::max({ std::abs(obj.scale.x), std::abs(obj.scale.y), std::abs(obj.scale.z) });
        if (maxDim < 0.1f) maxDim = 1.0f;
        sceneCameraDistance = std::clamp(maxDim * 3.0f, 2.5f, 35.0f);
        if (sceneRotationX < 10.0f) sceneRotationX = 20.0f;
    }
    else if (objIndex == -1) // Main Camera
    {
        sceneCameraTarget = mainCameraPos;
        sceneCameraDistance = 4.0f;
    }
    else
    {
        sceneCameraTarget = glm::vec3(0.0f);
        sceneCameraDistance = 5.0f;
    }
}

void GizmoSystem::draw3DObject(const Scene& scene, int objIndex, const glm::mat4& view, const glm::mat4& proj,
                               const ImVec2& offset, const ImVec2& size, ImU32 customColor, float thickness)
{
    const auto& sceneObjects = scene.getObjects();
    if (objIndex < 0 || objIndex >= static_cast<int>(sceneObjects.size())) return;

    const SceneObject& obj = sceneObjects[objIndex];
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 col = (customColor != 0) ? customColor : IM_COL32((int)(obj.color.r * 255), (int)(obj.color.g * 255), (int)(obj.color.b * 255), (int)(obj.color.a * 255));

    glm::mat4 model = scene.getWorldMatrix(objIndex);

    if (obj.type == ObjectType::CUBE || obj.meshId >= 0)
    {
        glm::vec3 localV[8] = {
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
            {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f, 0.5f,  0.5f}, {-0.5f, 0.5f,  0.5f}
        };
        ImVec2 projV[8];
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 worldPos = glm::vec3(model * glm::vec4(localV[i], 1.0f));
            projV[i] = projectPoint(worldPos, view, proj, offset, size);
        }

        auto drawEdge = [&](int idx1, int idx2) {
            if (projV[idx1].x > -90000.0f && projV[idx2].x > -90000.0f)
                drawList->AddLine(projV[idx1], projV[idx2], col, thickness);
        };

        drawEdge(0, 1); drawEdge(1, 2); drawEdge(2, 3); drawEdge(3, 0);
        drawEdge(4, 5); drawEdge(5, 6); drawEdge(6, 7); drawEdge(7, 4);
        drawEdge(0, 4); drawEdge(1, 5); drawEdge(2, 6); drawEdge(3, 7);
    }
    else if (obj.type == ObjectType::SPHERE)
    {
        const int numSegments = 16;
        ImVec2 prevXZ, prevXY, prevYZ;
        for (int i = 0; i <= numSegments; ++i)
        {
            float angle = (i * 2.0f * 3.14159f) / numSegments;
            
            glm::vec3 ptXZ(cos(angle) * 0.5f, 0.0f, sin(angle) * 0.5f);
            glm::vec3 wXZ = glm::vec3(model * glm::vec4(ptXZ, 1.0f));
            ImVec2 pXZ = projectPoint(wXZ, view, proj, offset, size);
            if (i > 0 && prevXZ.x > -90000.0f && pXZ.x > -90000.0f)
                drawList->AddLine(prevXZ, pXZ, col, 1.5f);
            prevXZ = pXZ;

            glm::vec3 ptXY(cos(angle) * 0.5f, sin(angle) * 0.5f, 0.0f);
            glm::vec3 wXY = glm::vec3(model * glm::vec4(ptXY, 1.0f));
            ImVec2 pXY = projectPoint(wXY, view, proj, offset, size);
            if (i > 0 && prevXY.x > -90000.0f && pXY.x > -90000.0f)
                drawList->AddLine(prevXY, pXY, col, 1.5f);
            prevXY = pXY;
        }
    }
    else if (obj.type == ObjectType::PLANE)
    {
        glm::vec3 localP[4] = {
            {-0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, 0.5f}, {-0.5f, 0.0f, 0.5f}
        };
        ImVec2 projP[4];
        for (int i = 0; i < 4; ++i)
        {
            glm::vec3 worldPos = glm::vec3(model * glm::vec4(localP[i], 1.0f));
            projP[i] = projectPoint(worldPos, view, proj, offset, size);
        }

        if (projP[0].x > -90000.0f && projP[1].x > -90000.0f && projP[2].x > -90000.0f && projP[3].x > -90000.0f)
        {
            drawList->AddQuadFilled(projP[0], projP[1], projP[2], projP[3], IM_COL32((int)(obj.color.r * 255), (int)(obj.color.g * 255), (int)(obj.color.b * 255), 35));
            drawList->AddQuad(projP[0], projP[1], projP[2], projP[3], col, 1.5f);
        }
    }
    else if (obj.type == ObjectType::LIGHT)
    {
        ImVec2 center = projectPoint(obj.position, view, proj, offset, size);
        if (center.x > -90000.0f)
        {
            drawList->AddCircleFilled(center, 4.0f, IM_COL32(255, 255, 100, 255));
            drawList->AddCircle(center, 8.0f, col, 0, 1.5f);
            
            glm::mat4 rotM = glm::mat4(1.0f);
            rotM = glm::rotate(rotM, glm::radians(obj.rotation.x), glm::vec3(1, 0, 0));
            rotM = glm::rotate(rotM, glm::radians(obj.rotation.y), glm::vec3(0, 1, 0));
            rotM = glm::rotate(rotM, glm::radians(obj.rotation.z), glm::vec3(0, 0, 1));
            glm::vec3 lookDir = glm::normalize(glm::vec3(rotM * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)));
            glm::vec3 right = glm::normalize(glm::vec3(rotM * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
            glm::vec3 up = glm::normalize(glm::vec3(rotM * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));

            float frustumDist = 0.6f;
            float frustumW = 0.35f;
            float frustumH = 0.35f;

            glm::vec3 baseCenter = obj.position + lookDir * frustumDist;
            glm::vec3 c0 = baseCenter - right * frustumW + up * frustumH;
            glm::vec3 c1 = baseCenter + right * frustumW + up * frustumH;
            glm::vec3 c2 = baseCenter + right * frustumW - up * frustumH;
            glm::vec3 c3 = baseCenter - right * frustumW - up * frustumH;

            ImVec2 sc0 = projectPoint(c0, view, proj, offset, size);
            ImVec2 sc1 = projectPoint(c1, view, proj, offset, size);
            ImVec2 sc2 = projectPoint(c2, view, proj, offset, size);
            ImVec2 sc3 = projectPoint(c3, view, proj, offset, size);

            ImU32 frustumCol = IM_COL32(255, 255, 100, 255);
            if (sc0.x > -90000.0f) drawList->AddLine(center, sc0, frustumCol, 1.5f);
            if (sc1.x > -90000.0f) drawList->AddLine(center, sc1, frustumCol, 1.5f);
            if (sc2.x > -90000.0f) drawList->AddLine(center, sc2, frustumCol, 1.5f);
            if (sc3.x > -90000.0f) drawList->AddLine(center, sc3, frustumCol, 1.5f);

            if (sc0.x > -90000.0f && sc1.x > -90000.0f) drawList->AddLine(sc0, sc1, frustumCol, 1.5f);
            if (sc1.x > -90000.0f && sc2.x > -90000.0f) drawList->AddLine(sc1, sc2, frustumCol, 1.5f);
            if (sc2.x > -90000.0f && sc3.x > -90000.0f) drawList->AddLine(sc2, sc3, frustumCol, 1.5f);
            if (sc3.x > -90000.0f && sc0.x > -90000.0f) drawList->AddLine(sc3, sc0, frustumCol, 1.5f);
            
            float t = (lookDir.y < -0.001f) ? (-obj.position.y / lookDir.y) : 10.0f;
            if (t < 0.0f || t > 50.0f) t = 10.0f;
            
            glm::vec3 endPt = obj.position + lookDir * t;
            ImVec2 end = projectPoint(endPt, view, proj, offset, size);
            
            if (end.x > -90000.0f)
            {
                drawList->AddLine(center, end, IM_COL32(255, 255, 100, 150), 1.0f);
                
                glm::vec3 p1 = endPt + glm::vec3(1, 0, 0);
                glm::vec3 p2 = endPt + glm::vec3(-1, 0, 0);
                glm::vec3 p3 = endPt + glm::vec3(0, 0, 1);
                glm::vec3 p4 = endPt + glm::vec3(0, 0, -1);
                ImVec2 sp1 = projectPoint(p1, view, proj, offset, size);
                ImVec2 sp2 = projectPoint(p2, view, proj, offset, size);
                ImVec2 sp3 = projectPoint(p3, view, proj, offset, size);
                ImVec2 sp4 = projectPoint(p4, view, proj, offset, size);
                if (sp1.x > -90000.0f && sp2.x > -90000.0f) drawList->AddLine(sp1, sp2, IM_COL32(255, 150, 0, 150), 1.5f);
                if (sp3.x > -90000.0f && sp4.x > -90000.0f) drawList->AddLine(sp3, sp4, IM_COL32(255, 150, 0, 150), 1.5f);
            }
        }
    }
}
