#pragma once

#include "core/Types.h"
#include "renderer/VulkanRenderer.h"
#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "scripting/ScriptEngine.h"
#include "editor/GizmoSystem.h"
#include "editor/ProfilerPanel.h"
#include "editor/AssetBrowserPanel.h"
#include "editor/SceneGenerator.h"
#include "editor/ModelDownloader.h"
#include "editor/EditorUI.h"
#include "physics/PhysicsSystem.h"

struct GLFWwindow;

class VulkanApp
{
public:
    static constexpr uint32_t WIDTH = 1280;
    static constexpr uint32_t HEIGHT = 720;

    VulkanApp();
    ~VulkanApp();

    void run();

private:
    void initWindow();
    void initEngine();
    void mainLoop();
    void cleanup();

    void update(float deltaTime);
    void render();

    void initializeDefaultScene();
    void toggleGameFullscreen();
    void updateWindowTitle();

    // GLFW Callbacks
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window = nullptr;
    GLFWwindow* secondaryWindow = nullptr;

    VulkanRenderer renderer;
    AssetManager assetManager;
    Scene scene;
    PhysEngine physEngine;
    ScriptEngine scriptEngine;
    GizmoSystem gizmoSystem;
    ProfilerPanel profilerPanel;
    AssetBrowserPanel assetBrowserPanel;
    SceneGenerator sceneGenerator;
    ModelDownloader modelDownloader;
    EditorUI editorUI;

    AppMode mode = AppMode::EDIT;
    int gameScore = 0;
    int highScore = 0;

    int savedWindowX = 100;
    int savedWindowY = 100;
    int savedWindowW = 1280;
    int savedWindowH = 720;

    float lastFrameTime = 0.0f;
};