#include "VulkanApp.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "editor/SceneGenerator.h"

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
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
    std::string defaultPath = "scene.json";
    if (!std::filesystem::exists(defaultPath))
    {
        std::vector<std::string> candidates = {
            "build/Release/scene.json",
            "dist/UnityHub3D_Vulkan/scene.json",
            "../scene.json",
            "../../scene.json"
        };
        for (const auto& c : candidates)
        {
            if (std::filesystem::exists(c))
            {
                defaultPath = c;
                break;
            }
        }
    }

    if (std::filesystem::exists(defaultPath))
    {
        scene.loadScene(defaultPath, &assetManager, assetManager.getCubeMeshId(), assetManager.getSphereMeshId(), assetManager.getPlaneMeshId(), &gizmoSystem, &renderer, &physEngine);
        return;
    }

    // Default fallback: Clean scene with a white plane
    printf("[VulkanApp] Initializing default scene with a white plane...\n");
    scene.initializeDefaultScene(assetManager.getCubeMeshId(), assetManager.getSphereMeshId(), assetManager.getPlaneMeshId(), -1, &physEngine);
    scene.saveScene("scene.json", &assetManager, &gizmoSystem, &renderer);
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

#ifdef _WIN32
static void terminateProcessTreeHelper(DWORD pid)
{
    if (pid == 0) return;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32W pe{};
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnapshot, &pe))
        {
            do
            {
                if (pe.th32ParentProcessID == pid)
                {
                    terminateProcessTreeHelper(pe.th32ProcessID);
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess)
    {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
}
#endif

void VulkanApp::update(float deltaTime)
{
    float dt = (deltaTime > 0.1f) ? 0.1f : deltaTime;
    scene.updateAnimations(dt);

    if (mode == AppMode::PLAY)
    {
#ifdef _WIN32
        static float jmeCheckTimer = 0.0f;
        jmeCheckTimer += deltaTime;
        if (jmeCheckTimer >= 0.5f)
        {
            jmeCheckTimer = 0.0f;
            bool jmeAlive = false;
            if (jmeProcessHandle)
            {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(static_cast<HANDLE>(jmeProcessHandle), &exitCode) && exitCode == STILL_ACTIVE)
                {
                    jmeAlive = true;
                }
            }
            if (!jmeAlive)
            {
                HWND hJme = FindWindowW(NULL, L"jMonkeyEngine - Trình Diễn Sinh Cảnh 3D (Cam View 2.4m x 2.0m)");
                if (!hJme)
                {
                    EnumWindows([](HWND hwnd, LPARAM lp) -> BOOL {
                        wchar_t title[256];
                        if (GetWindowTextW(hwnd, title, 256) > 0)
                        {
                            if (wcsstr(title, L"jMonkeyEngine") != nullptr)
                            {
                                *reinterpret_cast<bool*>(lp) = true;
                                return FALSE;
                            }
                        }
                        return TRUE;
                    }, reinterpret_cast<LPARAM>(&jmeAlive));
                }
                else
                {
                    jmeAlive = true;
                }
            }

            if (!jmeAlive)
            {
                printf("[VulkanApp] jMonkeyEngine window closed, returning to EDIT mode.\n");
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
    sceneCam.proj = glm::perspective(glm::radians(45.0f), (float)VulkanRenderer::WIDTH / (float)VulkanRenderer::HEIGHT, 0.1f, 500.0f);
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
                scene.initializeDefaultScene(assetManager.getCubeMeshId(), assetManager.getSphereMeshId(), assetManager.getPlaneMeshId(), -1, &physEngine);
                scene.saveScene("scene.json", &assetManager, &gizmoSystem, &renderer);
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
    terminateJmeProcess();

    HANDLE hJob = CreateJobObjectW(NULL, NULL);
    if (hJob)
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
        SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
        jmeJobHandle = hJob;
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

        DWORD createFlags = CREATE_SUSPENDED;
        if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, createFlags, NULL, workDirStr.c_str(), &si, &pi))
        {
            if (jmeJobHandle)
            {
                AssignProcessToJobObject(static_cast<HANDLE>(jmeJobHandle), pi.hProcess);
            }
            ResumeThread(pi.hThread);
            jmeProcessHandle = pi.hProcess;
            jmeProcessId = pi.dwProcessId;
            CloseHandle(pi.hThread);
            printf("[VulkanApp] Successfully launched jMonkeyEngine (PID %lu): %ls\n", pi.dwProcessId, cmdStr.c_str());
            return;
        }
    }

    // Fallback: khởi chạy qua run_jme.bat bằng CreateProcessW
    std::wstring batchCmd = L"cmd.exe /c run_jme.bat scene.json";
    std::vector<wchar_t> batchBuf(batchCmd.begin(), batchCmd.end());
    batchBuf.push_back(L'\0');

    STARTUPINFOW siBatch{};
    PROCESS_INFORMATION piBatch{};
    siBatch.cb = sizeof(siBatch);

    if (CreateProcessW(NULL, batchBuf.data(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, workDirStr.c_str(), &siBatch, &piBatch))
    {
        if (jmeJobHandle)
        {
            AssignProcessToJobObject(static_cast<HANDLE>(jmeJobHandle), piBatch.hProcess);
        }
        ResumeThread(piBatch.hThread);
        jmeProcessHandle = piBatch.hProcess;
        jmeProcessId = piBatch.dwProcessId;
        CloseHandle(piBatch.hThread);
        printf("[VulkanApp] Launched jMonkeyEngine via batch script in %ls (PID %lu)\n", workDirStr.c_str(), piBatch.dwProcessId);
        return;
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
    printf("[VulkanApp] Terminating jMonkeyEngine process tree and closing windows...\n");

    // 1. Terminate Job Object if active (this kills all processes in the job tree at OS kernel level)
    if (jmeJobHandle)
    {
        TerminateJobObject(static_cast<HANDLE>(jmeJobHandle), 0);
        CloseHandle(static_cast<HANDLE>(jmeJobHandle));
        jmeJobHandle = nullptr;
    }

    // 2. Terminate process tree via recursive Toolhelp32 & taskkill
    if (jmeProcessId != 0)
    {
        terminateProcessTreeHelper(jmeProcessId);
        std::wstring killCmd = L"taskkill /PID " + std::to_wstring(jmeProcessId) + L" /T /F >nul 2>&1";
        _wsystem(killCmd.c_str());
    }

    // 3. Fallback direct handle kill
    if (jmeProcessHandle)
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(static_cast<HANDLE>(jmeProcessHandle), &exitCode) && exitCode == STILL_ACTIVE)
        {
            TerminateProcess(static_cast<HANDLE>(jmeProcessHandle), 0);
        }
        CloseHandle(static_cast<HANDLE>(jmeProcessHandle));
        jmeProcessHandle = nullptr;
    }
    jmeProcessId = 0;

    // 4. Forcibly kill any running Jme_scene.exe
    _wsystem(L"taskkill /IM Jme_scene.exe /T /F >nul 2>&1");

    // 5. Find and terminate any jME windows by title
    EnumWindows([](HWND hwnd, LPARAM) -> BOOL {
        wchar_t title[512] = { 0 };
        if (GetWindowTextW(hwnd, title, 512) > 0)
        {
            if (wcsstr(title, L"jMonkeyEngine") != nullptr || wcsstr(title, L"Cam View 2.4m x 2.0m") != nullptr)
            {
                DWORD pid = 0;
                GetWindowThreadProcessId(hwnd, &pid);
                if (pid != 0 && pid != GetCurrentProcessId())
                {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                    if (hProc)
                    {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                }
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
            }
        }
        return TRUE;
    }, 0);

    // 6. Kill any Java/javaw process running com.mygame.Main or from Jme_scene
    _wsystem(L"powershell -NoProfile -NonInteractive -Command \"Get-CimInstance Win32_Process | Where-Object { ($_.Name -like '*java*') -and ($_.CommandLine -match 'com.mygame.Main|Jme_scene|jme-scene|scene.json') } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }\" >nul 2>&1");

    printf("[VulkanApp] jMonkeyEngine completely terminated.\n");
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
