#pragma once

#include "core/Types.h"
#include <string>
#include <functional>
#include <vulkan/vulkan.h>
#include "imgui.h"

struct GLFWwindow;
class VulkanRenderer;
class Scene;
class AssetManager;
class ScriptEngine;
class GizmoSystem;
class ProfilerPanel;
class AssetBrowserPanel;
class SceneGenerator;
class ModelDownloader;
class PhysEngine;

class EditorUI
{
public:
    EditorUI();
    ~EditorUI();

    void init(GLFWwindow* window, VulkanRenderer* renderer);
    void cleanup(VulkanRenderer* renderer);
    void setupUnityStyle();

    void render(GLFWwindow* window,
                VulkanRenderer* renderer,
                Scene* scene,
                AssetManager* assetManager,
                ScriptEngine* scriptEngine,
                GizmoSystem* gizmoSystem,
                ProfilerPanel* profilerPanel,
                AssetBrowserPanel* assetBrowserPanel,
                SceneGenerator* sceneGenerator,
                ModelDownloader* modelDownloader,
                PhysEngine* physEngine,
                AppMode& mode,
                int& gameScore,
                int highScore,
                const std::string& selectedGpuName,
                std::function<void()> onPlay,
                std::function<void()> onStop,
                std::function<void()> onReloadLua,
                std::function<void()> onResetScene,
                std::function<void()> onSaveScene,
                std::function<void()> onLoadScene,
                std::function<void()> onToggleFullscreen);

    VkDescriptorSet getOffscreenDescriptorSet() const { return offscreenDescriptorSet; }
    VkDescriptorSet getGameViewDescriptorSet() const { return gameViewDescriptorSet; }

    float getLeftPanelWidth() const { return leftPanelWidth; }
    float getRightPanelWidth() const { return rightPanelWidth; }
    float getBottomPanelHeight() const { return bottomPanelHeight; }

private:
    void drawMainMenuBar(Scene* scene, AppMode& mode, int& gameScore, const std::string& selectedGpuName,
                         std::function<void()> onPlay, std::function<void()> onStop,
                         std::function<void()> onReloadLua, std::function<void()> onResetScene,
                         std::function<void()> onSaveScene, std::function<void()> onLoadScene,
                         GizmoSystem* gizmoSystem, ProfilerPanel* profilerPanel,
                         AssetBrowserPanel* assetBrowserPanel, SceneGenerator* sceneGenerator,
                         ModelDownloader* modelDownloader);

    void drawHierarchyPanel(Scene* scene, AssetManager* assetManager, ScriptEngine* scriptEngine,
                            GizmoSystem* gizmoSystem, AppMode mode, SceneGenerator* sceneGenerator,
                            float menuBarHeight, float centerHeight);

    void drawInspectorPanel(Scene* scene, AssetManager* assetManager, ScriptEngine* scriptEngine,
                            VulkanRenderer* renderer, GizmoSystem* gizmoSystem, AppMode mode,
                            float menuBarHeight, float centerHeight, float editorWidth);

    void drawConsolePanel(const std::string& selectedGpuName, AppMode mode, const Scene* scene,
                          float editorHeight, float windowWidth);

    VkDescriptorPool imguiDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet offscreenDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorSet gameViewDescriptorSet = VK_NULL_HANDLE;

    float leftPanelWidth = 260.0f;
    float rightPanelWidth = 320.0f;
    float bottomPanelHeight = 220.0f;
};
