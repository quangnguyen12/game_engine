#include "VulkanApp.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

VulkanApp::VulkanApp()
{
}

VulkanApp::~VulkanApp()
{
}

void VulkanApp::run()
{
    initWindow();
    initEngine();
    mainLoop();
    cleanup();
}

void VulkanApp::initWindow()
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Unity Hub 3D Vulkan Engine - [EDIT MODE]", nullptr, nullptr);
    if (!window)
    {
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
}

void VulkanApp::initEngine()
{
    renderer.init(window);
    assetManager.init(&renderer);
    editorUI.init(window, &renderer);
    physEngine.init();

    // Create default primitive meshes
    assetManager.createCubeMesh();
    assetManager.createSphereMesh();
    assetManager.createPlaneMesh();
    assetManager.createCylinderMesh();
    assetManager.createConeMesh();

    initializeDefaultScene();
    scriptEngine.init(window, &scene, &gameScore, &highScore);
}

void VulkanApp::initializeDefaultScene()
{
    scene.initializeDefaultScene(
        assetManager.getCubeMeshId(),
        assetManager.getSphereMeshId(),
        assetManager.getPlaneMeshId(),
        -1,
        &physEngine
    );
}

void VulkanApp::updateWindowTitle()
{
    if (!window) return;
    std::string title = (mode == AppMode::PLAY)
        ? "Unity Hub 3D Vulkan Engine - [PLAY MODE >]"
        : "Unity Hub 3D Vulkan Engine - [EDIT MODE]";
    glfwSetWindowTitle(window, title.c_str());
}

void VulkanApp::toggleGameFullscreen()
{
    gizmoSystem.isGameFullscreen = !gizmoSystem.isGameFullscreen;

    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    if (gizmoSystem.isGameFullscreen)
    {
        glfwGetWindowPos(window, &savedWindowX, &savedWindowY);
        glfwGetWindowSize(window, &savedWindowW, &savedWindowH);

        if (monitors && monitorCount > 1)
        {
            int m1x = 0, m1y = 0;
            glfwGetMonitorPos(monitors[0], &m1x, &m1y);
            const GLFWvidmode* mode1 = glfwGetVideoMode(monitors[0]);
            const GLFWvidmode* mode2 = glfwGetVideoMode(monitors[1]);

            if (mode1 && mode2)
            {
                int totalW = mode1->width + mode2->width;
                int totalH = std::max(mode1->height, mode2->height);

                glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
                glfwSetWindowPos(window, m1x, m1y);
                glfwSetWindowSize(window, totalW, totalH);
            }
        }
        else
        {
            glfwMaximizeWindow(window);
        }
    }
    else
    {
        glfwRestoreWindow(window);
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowPos(window, savedWindowX, savedWindowY);
        glfwSetWindowSize(window, savedWindowW, savedWindowH);
    }
}

void VulkanApp::update(float deltaTime)
{
    if (mode == AppMode::PLAY)
    {
        float dt = (deltaTime > 0.1f) ? 0.1f : deltaTime;
        scriptEngine.update(&scene, &physEngine, dt);
        physEngine.update(dt);
        scene.syncPhysicsToTransform(&physEngine);

        // Update main camera target to follow player cube
        for (const auto& obj : scene.getObjects())
        {
            if (obj.name == "Player Cube")
            {
                gizmoSystem.mainCameraTarget = obj.position;
                gizmoSystem.mainCameraPos = obj.position + glm::vec3(2.5f, 2.5f, 2.5f);
                break;
            }
        }
    }
}

void VulkanApp::render()
{
    CameraData sceneCam;
    sceneCam.proj = glm::perspective(glm::radians(45.0f), (float)VulkanRenderer::WIDTH / (float)VulkanRenderer::HEIGHT, 0.1f, 100.0f);
    sceneCam.proj[1][1] *= -1;

    glm::vec3 offset(
        gizmoSystem.sceneCameraDistance * cos(glm::radians(gizmoSystem.sceneRotationX)) * sin(glm::radians(gizmoSystem.sceneRotationY)),
        gizmoSystem.sceneCameraDistance * sin(glm::radians(gizmoSystem.sceneRotationX)),
        gizmoSystem.sceneCameraDistance * cos(glm::radians(gizmoSystem.sceneRotationX)) * cos(glm::radians(gizmoSystem.sceneRotationY))
    );
    glm::vec3 camPos = gizmoSystem.sceneCameraTarget + offset;
    sceneCam.view = glm::lookAt(camPos, gizmoSystem.sceneCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    sceneCam.viewPos = camPos;

    CameraData gameCam;
    gameCam.proj = glm::perspective(glm::radians(gizmoSystem.mainCameraFov), (float)VulkanRenderer::WIDTH / (float)VulkanRenderer::HEIGHT, gizmoSystem.mainCameraNear, gizmoSystem.mainCameraFar);
    gameCam.proj[1][1] *= -1;
    gameCam.view = glm::lookAt(gizmoSystem.mainCameraPos, gizmoSystem.mainCameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    gameCam.viewPos = gizmoSystem.mainCameraPos;

    renderer.updateUniformBuffer(renderer.getCurrentFrame(), scene, sceneCam, gameCam);

    renderer.drawFrame(scene, assetManager, [this]() {
        editorUI.render(
            window,
            &renderer,
            &scene,
            &assetManager,
            &scriptEngine,
            &gizmoSystem,
            &profilerPanel,
            &assetBrowserPanel,
            &sceneGenerator,
            &modelDownloader,
            &physEngine,
            mode,
            gameScore,
            highScore,
            renderer.getGpuName(),
            // onPlay
            [this]() {
                mode = AppMode::PLAY;
                updateWindowTitle();
                scene.savePlayModeState();
                scene.initPhysicsBodies(&physEngine);
                scriptEngine.reloadLuaScripts(&scene);
                gameScore = 0;
            },
            // onStop
            [this]() {
                mode = AppMode::EDIT;
                updateWindowTitle();
                scene.restoreEditModeState(&physEngine);
                for (auto& obj : scene.getObjects())
                    obj.luaInstances.clear();
            },
            // onReloadLua
            [this]() {
                scriptEngine.reloadLuaScripts(&scene);
            },
            // onResetScene
            [this]() {
                initializeDefaultScene();
                gameScore = 0;
            },
            // onSaveScene
            [this]() {
                scene.saveScene("scene.json");
            },
            // onLoadScene
            [this]() {
                scene.loadScene("scene.json", assetManager.getCubeMeshId(), assetManager.getSphereMeshId(), assetManager.getPlaneMeshId());
            },
            // onToggleFullscreen
            [this]() {
                toggleGameFullscreen();
            }
        );
    });
}

void VulkanApp::mainLoop()
{
    lastFrameTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        update(deltaTime);
        profilerPanel.update(deltaTime, renderer.getPhysicalDevice());
        render();
    }

    vkDeviceWaitIdle(renderer.getDevice());
}

void VulkanApp::cleanup()
{
    if (secondaryWindow)
    {
        glfwDestroyWindow(secondaryWindow);
        secondaryWindow = nullptr;
    }

    editorUI.cleanup(&renderer);
    assetManager.cleanup();
    renderer.cleanup();

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}

void VulkanApp::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->renderer.setFramebufferResized(true);
    }
}

void VulkanApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (!app) return;

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_F5)
        {
            if (app->mode == AppMode::PLAY)
            {
                app->mode = AppMode::EDIT;
                app->updateWindowTitle();
                app->scene.restoreEditModeState(&app->physEngine);
                for (auto& obj : app->scene.getObjects())
                    obj.luaInstances.clear();
            }
            else
            {
                app->mode = AppMode::PLAY;
                app->updateWindowTitle();
                app->scene.savePlayModeState();
                app->scene.initPhysicsBodies(&app->physEngine);
                app->scriptEngine.reloadLuaScripts(&app->scene);
                app->gameScore = 0;
            }
        }
        else if (key == GLFW_KEY_F11)
        {
            app->toggleGameFullscreen();
        }
    }
}
