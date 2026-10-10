#include "VulkanApp.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
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

    // Default primitive meshes are created inside assetManager.init(&renderer)

    initializeDefaultScene();
    scriptEngine.init(window, &scene, &gameScore, &highScore);
}

void VulkanApp::initializeDefaultScene()
{
    int groundTexId = assetManager.getOrLoadTextureAsset("assets/grass.jpg");
    scene.initializeDefaultScene(
        assetManager.getCubeMeshId(),
        assetManager.getSphereMeshId(),
        assetManager.getPlaneMeshId(),
        groundTexId,
        &physEngine
    );
    gizmoSystem.frameScenicPhotoView(scene, 0);
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
#ifdef _WIN32
        if (jmeProcessHandle)
        {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(static_cast<HANDLE>(jmeProcessHandle), &exitCode) && exitCode != STILL_ACTIVE)
            {
                printf("[VulkanApp] jMonkeyEngine window closed (exit code %lu), returning to EDIT mode.\n", exitCode);
                CloseHandle(static_cast<HANDLE>(jmeProcessHandle));
                jmeProcessHandle = nullptr;
                jmeProcessId = 0;
                stopPlayMode();
                return;
            }
        }
#endif
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
    float gameAspect = gizmoSystem.getGameViewAspectRatio();
    gameCam.proj = glm::perspective(glm::radians(gizmoSystem.mainCameraFov), gameAspect, gizmoSystem.mainCameraNear, gizmoSystem.mainCameraFar);
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
                startPlayMode();
            },
            // onStop
            [this]() {
                stopPlayMode();
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
                std::string targetPath = scene.getCurrentScenePath();
                if (targetPath.empty())
                {
                    targetPath = Scene::getEnvironmentFileName(scene.getCurrentEnvironmentName());
                }
                scene.saveScene(targetPath, &assetManager, &gizmoSystem, &renderer);
                if (targetPath != "scene.json")
                {
                    scene.saveScene("scene.json", &assetManager, &gizmoSystem, &renderer);
                    scene.setCurrentScenePath(targetPath);
                }
            },
            // onLoadScene
            [this]() {
                std::string loadPath = scene.getCurrentScenePath();
                if (loadPath.empty()) loadPath = "scene.json";
                scene.loadScene(loadPath, &assetManager, assetManager.getCubeMeshId(), assetManager.getSphereMeshId(), assetManager.getPlaneMeshId(), &gizmoSystem, &renderer, &physEngine);
                if (mode == AppMode::PLAY)
                {
                    scene.initPhysicsBodies(&physEngine);
                    scriptEngine.reloadLuaScripts(&scene);
                }
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
    terminateJmeProcess();

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

void VulkanApp::startPlayMode()
{
    mode = AppMode::PLAY;
    updateWindowTitle();
    scene.savePlayModeState();
    scene.initPhysicsBodies(&physEngine);
    scriptEngine.reloadLuaScripts(&scene);
    gameScore = 0;

    // 1. Tự động xuất và đồng bộ scene hiện tại sang jMonkeyEngine
    scene.exportToJMonkeyEngine("scene_jme.json", &assetManager, &gizmoSystem, &renderer);

    gizmoSystem.lastExportedImagePath = "jMonkeyEngine (Đang chạy...)";
    gizmoSystem.exportNotificationTimer = 4.0f;

    // 2. Khởi chạy tiến trình jMonkeyEngine trực tiếp
    launchJmeProcess();
}

void VulkanApp::stopPlayMode()
{
    mode = AppMode::EDIT;
    updateWindowTitle();
    scene.restoreEditModeState(&physEngine);
    for (auto& obj : scene.getObjects())
        obj.luaInstances.clear();

    terminateJmeProcess();
}

void VulkanApp::launchJmeProcess()
{
#ifdef _WIN32
    if (jmeProcessHandle)
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(static_cast<HANDLE>(jmeProcessHandle), &exitCode) && exitCode == STILL_ACTIVE)
        {
            return;
        }
        CloseHandle(static_cast<HANDLE>(jmeProcessHandle));
        jmeProcessHandle = nullptr;
        jmeProcessId = 0;
    }

    std::filesystem::path jmeDir = "../Jme_scene";
    if (!std::filesystem::exists(jmeDir))
    {
        jmeDir = "D:/Game_prj/Jme_scene";
    }

    std::filesystem::path jmeExe = jmeDir / "Jme_scene.exe";
    if (!std::filesystem::exists(jmeExe))
    {
        jmeExe = "../Jme_scene.exe";
    }
    if (!std::filesystem::exists(jmeExe))
    {
        jmeExe = "D:/Game_prj/Jme_scene.exe";
    }

    std::wstring workDirStr = std::filesystem::absolute(jmeDir).wstring();

    if (std::filesystem::exists(jmeExe))
    {
        std::wstring exeStr = std::filesystem::absolute(jmeExe).wstring();
        std::wstring cmdStr = L"\"" + exeStr + L"\" scene.json";

        STARTUPINFOW si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);

        std::vector<wchar_t> cmdBuf(cmdStr.begin(), cmdStr.end());
        cmdBuf.push_back(L'\0');

        if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, workDirStr.c_str(), &si, &pi))
        {
            jmeProcessHandle = pi.hProcess;
            jmeProcessId = pi.dwProcessId;
            CloseHandle(pi.hThread);
            printf("[VulkanApp] Successfully launched jMonkeyEngine (PID %lu): %ls\n", pi.dwProcessId, cmdStr.c_str());
            return;
        }
    }

    // Fallback: khởi chạy qua run_jme.bat
    std::wstring batchCmd = L"/c cd /d \"" + workDirStr + L"\" && run_jme.bat";
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = batchCmd.c_str();
    sei.nShow = SW_SHOW;

    if (ShellExecuteExW(&sei))
    {
        jmeProcessHandle = sei.hProcess;
        printf("[VulkanApp] Launched jMonkeyEngine via batch script in %ls\n", workDirStr.c_str());
    }
    else
    {
        printf("[VulkanApp] Failed to launch jMonkeyEngine!\n");
    }
#endif
}

void VulkanApp::terminateJmeProcess()
{
#ifdef _WIN32
    if (jmeProcessHandle)
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(static_cast<HANDLE>(jmeProcessHandle), &exitCode) && exitCode == STILL_ACTIVE)
        {
            TerminateProcess(static_cast<HANDLE>(jmeProcessHandle), 0);
            printf("[VulkanApp] Terminated jMonkeyEngine process.\n");
        }
        CloseHandle(static_cast<HANDLE>(jmeProcessHandle));
        jmeProcessHandle = nullptr;
        jmeProcessId = 0;
    }
#endif
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
                app->stopPlayMode();
            }
            else
            {
                app->startPlayMode();
            }
        }
        else if (key == GLFW_KEY_F11)
        {
            app->toggleGameFullscreen();
        }
    }
}
