#include "editor/EditorUI.h"

#include "renderer/VulkanRenderer.h"

#include "scene/Scene.h"

#include "assets/AssetManager.h"

#include "scripting/ScriptEngine.h"

#include "editor/GizmoSystem.h"

#include "editor/ProfilerPanel.h"

#include "editor/AssetBrowserPanel.h"

#include "editor/SceneGenerator.h"

#include "editor/ModelDownloader.h"

#include "physics/PhysicsSystem.h"

#include "vendor/tinyfiledialogs.h"

#include "animation/AnimationLoader.h"

#include "animation/AnimatorComponent.h"

#include "imgui_impl_glfw.h"

#include "imgui_impl_vulkan.h"



#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <stdexcept>

#include <fstream>

#include <cstring>

#include <iostream>

#include <filesystem>

#include <unordered_map>

#include <algorithm>

#include <cstdlib>



static bool extractZipArchive(const std::string& zipPath, const std::string& destDir)

{

    std::filesystem::create_directories(destDir);

    // Use Windows native tar command

    std::string cmd = "tar -xf \"" + zipPath + "\" -C \"" + destDir + "\"";

    int ret = std::system(cmd.c_str());

    if (ret == 0) return true;



    // Fallback to powershell Expand-Archive

    std::string psCmd = "powershell -NoProfile -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + destDir + "' -Force\"";

    ret = std::system(psCmd.c_str());

    return ret == 0;

}



static void importZipAssetPack(const std::string& zipPath, AssetManager* assetMgr, Scene* scene, AssetBrowserPanel* assetBrowserPanel = nullptr)

{

    std::filesystem::path zPath(zipPath);

    std::string packStem = zPath.stem().string();

    std::string tempExtract = "temp_zip_extract/" + packStem;



    if (!extractZipArchive(zipPath, tempExtract)) {

        tinyfd_messageBox("Lỗi giải nén ZIP", "Không thể giải nén tệp zip. Hãy kiểm tra đường dẫn và tệp nguồn.", "ok", "error", 1);

        return;

    }



    std::filesystem::create_directories("assets/models");

    std::filesystem::create_directories("assets/animations");

    std::filesystem::create_directories("assets/textures");



    int modelCount = 0;

    int animCount = 0;

    int texCount = 0;

    std::vector<std::string> extractedAnimPaths;

    std::string firstModelPath;



    for (const auto& entry : std::filesystem::recursive_directory_iterator(tempExtract))

    {

        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();

        for (char& c : ext) c = static_cast<char>(tolower(c));

        std::string fname = entry.path().filename().string();



        if (ext == ".fbx")

        {

            if (AnimationLoader::isAnimationOnlyFBX(entry.path().string()))

            {

                std::string dest = "assets/animations/" + fname;

                std::filesystem::copy_file(entry.path(), dest, std::filesystem::copy_options::overwrite_existing);

                extractedAnimPaths.push_back(dest);

                animCount++;

            }

            else

            {

                std::string dest = "assets/models/" + fname;

                std::filesystem::copy_file(entry.path(), dest, std::filesystem::copy_options::overwrite_existing);

                if (firstModelPath.empty()) firstModelPath = dest;

                modelCount++;

            }

        }

        else if (ext == ".obj" || ext == ".glb" || ext == ".gltf")

        {

            std::string dest = "assets/models/" + fname;

            std::filesystem::copy_file(entry.path(), dest, std::filesystem::copy_options::overwrite_existing);

            if (firstModelPath.empty()) firstModelPath = dest;

            modelCount++;

        }

        else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga")

        {

            std::string dest = "assets/textures/" + fname;

            std::filesystem::copy_file(entry.path(), dest, std::filesystem::copy_options::overwrite_existing);

            texCount++;

        }

    }



    std::error_code ec;

    std::filesystem::remove_all("temp_zip_extract", ec);



    if (assetBrowserPanel) {

        assetBrowserPanel->refreshAssetRegistry(assetMgr);

    }



    auto selectedObj = scene ? scene->getSelectedObject() : nullptr;

    auto animComp = selectedObj ? selectedObj->getAnimator() : nullptr;

    if (animComp && !extractedAnimPaths.empty())

    {

        for (const auto& ap : extractedAnimPaths)

        {

            auto clip = AnimationLoader::loadFBXClip(ap, animComp->skeleton.get());

            if (clip) {

                animComp->addClip(clip);

            }

        }

        if (!animComp->isPlaying && !animComp->clipNames.empty()) {

            animComp->play(animComp->clipNames.back(), true);

        }

    }



    std::string summary = "Đã import thành công gói '" + packStem + "':\n- 3D Models: " +

                          std::to_string(modelCount) + "\n- Animations: " +

                          std::to_string(animCount) + "\n- Textures: " +

                          std::to_string(texCount) + "\n\nTất cả tài nguyên đã sẵn sàng trong Asset Browser và thư mục assets/!";

    tinyfd_messageBox("Import Pack Thành Công", summary.c_str(), "ok", "info", 1);

}



EditorUI::EditorUI()

{

}



EditorUI::~EditorUI()

{

}



void EditorUI::setupUnityStyle()

{

    ImGuiStyle& style = ImGui::GetStyle();



    style.WindowRounding = 6.0f;

    style.ChildRounding = 4.0f;

    style.FrameRounding = 4.0f;

    style.PopupRounding = 4.0f;

    style.ScrollbarRounding = 4.0f;

    style.GrabRounding = 3.0f;

    style.TabRounding = 4.0f;



    style.WindowPadding = ImVec2(10.0f, 10.0f);

    style.FramePadding = ImVec2(8.0f, 4.0f);

    style.ItemSpacing = ImVec2(8.0f, 6.0f);



    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                  = ImVec4(0.92f, 0.93f, 0.94f, 1.00f);

    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.52f, 0.54f, 1.00f);

    colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.17f, 0.94f);

    colors[ImGuiCol_ChildBg]               = ImVec4(0.16f, 0.17f, 0.20f, 0.80f);

    colors[ImGuiCol_PopupBg]               = ImVec4(0.15f, 0.16f, 0.19f, 0.96f);

    colors[ImGuiCol_Border]                = ImVec4(0.24f, 0.26f, 0.30f, 0.60f);

    colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);

    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.28f, 0.30f, 0.36f, 1.00f);

    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.34f, 0.36f, 0.42f, 1.00f);

    colors[ImGuiCol_TitleBg]               = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);

    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);

    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);

    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.12f, 0.13f, 0.15f, 0.60f);

    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.26f, 0.28f, 0.34f, 1.00f);

    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.34f, 0.36f, 0.44f, 1.00f);

    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.40f, 0.42f, 0.50f, 1.00f);

    colors[ImGuiCol_CheckMark]             = ImVec4(0.00f, 0.58f, 0.96f, 1.00f);

    colors[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.58f, 0.96f, 1.00f);

    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.18f, 0.68f, 1.00f, 1.00f);

    colors[ImGuiCol_Button]                = ImVec4(0.22f, 0.24f, 0.29f, 1.00f);

    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.00f, 0.52f, 0.88f, 1.00f);

    colors[ImGuiCol_ButtonActive]          = ImVec4(0.00f, 0.44f, 0.76f, 1.00f);

    colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);

    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.00f, 0.52f, 0.88f, 0.80f);

    colors[ImGuiCol_HeaderActive]          = ImVec4(0.00f, 0.44f, 0.76f, 1.00f);

    colors[ImGuiCol_Separator]             = ImVec4(0.25f, 0.27f, 0.32f, 1.00f);

    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);

    colors[ImGuiCol_TabHovered]            = ImVec4(0.00f, 0.52f, 0.88f, 0.80f);

    colors[ImGuiCol_TabActive]             = ImVec4(0.22f, 0.25f, 0.30f, 1.00f);

}



void EditorUI::init(GLFWwindow* window, VulkanRenderer* renderer)

{

    VkDescriptorPoolSize pool_sizes[] = {

        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },

        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },

        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },

        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },

        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },

        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },

        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },

        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },

        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },

        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },

        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }

    };



    VkDescriptorPoolCreateInfo pool_info{};

    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    pool_info.maxSets = 1000;

    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);

    pool_info.pPoolSizes = pool_sizes;



    if (vkCreateDescriptorPool(renderer->getDevice(), &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS)

    {

        throw std::runtime_error("Failed to create ImGui descriptor pool!");

    }



    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;



    // Load Windows system font with full Vietnamese UTF-8 glyph support

    ImFontConfig fontConfig;

    fontConfig.OversampleH = 2;

    fontConfig.OversampleV = 2;

    if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf")) {

        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f, &fontConfig, io.Fonts->GetGlyphRangesVietnamese());

    } else if (std::filesystem::exists("C:/Windows/Fonts/arial.ttf")) {

        io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 16.0f, &fontConfig, io.Fonts->GetGlyphRangesVietnamese());

    } else {

        io.Fonts->AddFontDefault();

    }



    setupUnityStyle();



    QueueFamilyIndices indices = renderer->findQueueFamilies(renderer->getPhysicalDevice());



    ImGui_ImplGlfw_InitForVulkan(window, true);



    ImGui_ImplVulkan_InitInfo init_info{};

    init_info.Instance = renderer->getInstance();

    init_info.PhysicalDevice = renderer->getPhysicalDevice();

    init_info.Device = renderer->getDevice();

    init_info.QueueFamily = indices.graphicsFamily.value();

    init_info.Queue = renderer->getGraphicsQueue();

    init_info.DescriptorPool = imguiDescriptorPool;

    init_info.RenderPass = renderer->getRenderPass();

    init_info.MinImageCount = VulkanRenderer::MAX_FRAMES_IN_FLIGHT;

    init_info.ImageCount = static_cast<uint32_t>(renderer->getSwapChainImages().size());

    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;



    ImGui_ImplVulkan_Init(&init_info);



    // Scene view texture

    offscreenDescriptorSet = ImGui_ImplVulkan_AddTexture(

        renderer->getOffscreenSampler(),

        renderer->getOffscreenColorImageView(),

        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    );



    // Game view texture

    gameViewDescriptorSet = ImGui_ImplVulkan_AddTexture(

        renderer->getGameSampler(),

        renderer->getGameColorImageView(),

        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

    );



    renderer->setOffscreenDescriptorSet(offscreenDescriptorSet);

    renderer->setGameViewDescriptorSet(gameViewDescriptorSet);

}



void EditorUI::cleanup(VulkanRenderer* renderer)

{

    ImGui_ImplVulkan_Shutdown();

    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();



    if (imguiDescriptorPool != VK_NULL_HANDLE && renderer)

    {

        vkDestroyDescriptorPool(renderer->getDevice(), imguiDescriptorPool, nullptr);

        imguiDescriptorPool = VK_NULL_HANDLE;

    }

}



void EditorUI::render(GLFWwindow* window,

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

                      std::function<void()> onToggleFullscreen)

{

    ImGui_ImplVulkan_NewFrame();

    ImGui_ImplGlfw_NewFrame();

    ImGui::NewFrame();



    auto& sceneObjects = scene->getObjects();

    int selectedObjectIndex = scene->getSelectedObjectIndex();



    // Handle editor hotkeys for gizmo switching

    if (mode == AppMode::EDIT)

    {

        if (!ImGui::GetIO().WantTextInput)

        {

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))

            {

                scene->undo();

            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))

            {

                scene->redo();

            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))

            {

                if (ImGui::GetIO().KeyShift)

                {

                    showSaveSceneModal = true;

                }

                else

                {

                    if (onSaveScene) onSaveScene();

                }

            }

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L))

            {

                showLoadSceneModal = true;

            }

        }



        if (!ImGui::GetIO().WantTextInput && (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)))

        {

            if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))

            {

                scene->saveHistory();

                sceneObjects.erase(sceneObjects.begin() + selectedObjectIndex);

                scene->setSelectedObjectIndex(-1);

                selectedObjectIndex = -1;

            }

        }

        

        if (ImGui::IsKeyPressed(ImGuiKey_Q)) gizmoSystem->activeGizmo = GizmoType::HAND;

        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoSystem->activeGizmo = GizmoType::TRANSLATE;

        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoSystem->activeGizmo = GizmoType::ROTATE;

        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoSystem->activeGizmo = GizmoType::SCALE;

        if (ImGui::IsKeyPressed(ImGuiKey_T)) gizmoSystem->activeGizmo = GizmoType::RECT;

        if (ImGui::IsKeyPressed(ImGuiKey_Y)) gizmoSystem->activeGizmo = GizmoType::TRANSFORM_COMBINED;



        // Focus camera on selected object (F in Unity/Maya, Numpad . / Period in Blender)

        if (!ImGui::GetIO().WantTextInput && (ImGui::IsKeyPressed(ImGuiKey_F) || ImGui::IsKeyPressed(ImGuiKey_KeypadDecimal) || ImGui::IsKeyPressed(ImGuiKey_Period)))

        {

            if (gizmoSystem && (selectedObjectIndex == -1 || (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))))

            {

                gizmoSystem->focusOnObject(selectedObjectIndex, *scene);

            }

        }



        if (ImGui::IsKeyPressed(ImGuiKey_G) && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))

        {

            gizmoSystem->isBlenderGrabMode = !gizmoSystem->isBlenderGrabMode;

            if (gizmoSystem->isBlenderGrabMode)

            {

                scene->saveHistory();

                gizmoSystem->grabStartPos = sceneObjects[selectedObjectIndex].position;

                gizmoSystem->grabConstrainAxis = -1;

            }

        }



        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && gizmoSystem->isGameFullscreen)

        {

            if (onToggleFullscreen) onToggleFullscreen();

        }

    }



    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    float windowWidth = static_cast<float>(width);

    float windowHeight = static_cast<float>(height);



    int monitorCount = 0;

    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    float editorWidth = windowWidth;

    float editorHeight = windowHeight;



    if (gizmoSystem->isGameFullscreen && monitors && monitorCount > 1)

    {

        const GLFWvidmode* mode1 = glfwGetVideoMode(monitors[0]);

        const GLFWvidmode* mode2 = glfwGetVideoMode(monitors[1]);

        if (mode1 && mode2)

        {

            editorWidth = static_cast<float>(mode1->width);

            editorHeight = static_cast<float>(mode1->height);



            float m2Width = static_cast<float>(mode2->width);

            float m2Height = static_cast<float>(mode2->height);



            ImGui::SetNextWindowPos(ImVec2(editorWidth, 0.0f), ImGuiCond_Always);

            ImGui::SetNextWindowSize(ImVec2(m2Width, m2Height), ImGuiCond_Always);

            if (ImGui::Begin("Game View Fullscreen (Monitor 2)", &gizmoSystem->isGameFullscreen, 

                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))

            {

                gizmoSystem->drawGameView(gameViewDescriptorSet, ImGui::GetWindowPos(), ImGui::GetWindowSize(), gameScore, highScore, onToggleFullscreen, mode, nullptr, renderer);

            }

            ImGui::End();

        }

    }



    float menuBarHeight = 25.0f;

    float effectiveBottomHeight = assetBrowserPanel->getVisible() ? bottomPanelHeight : 0.0f;

    float centerHeight = editorHeight - menuBarHeight - effectiveBottomHeight;

    float centerWidth = editorWidth - leftPanelWidth - rightPanelWidth;



    if (centerWidth < 100.0f) centerWidth = 100.0f;

    if (centerHeight < 100.0f) centerHeight = 100.0f;



    // 1. Top Menu Bar

    drawMainMenuBar(scene, mode, gameScore, selectedGpuName,

                    onPlay, onStop, onReloadLua, onResetScene, onSaveScene, onLoadScene,

                    gizmoSystem, profilerPanel, assetBrowserPanel, sceneGenerator, modelDownloader, renderer, assetManager);



    // 2. Hierarchy Panel

    drawHierarchyPanel(scene, assetManager, scriptEngine, gizmoSystem, mode, sceneGenerator, menuBarHeight, centerHeight);



    // 3. Scene View

    float sceneWidth = (gizmoSystem->showGameViewWindow && !gizmoSystem->isGameViewDetached) ? centerWidth * 0.5f : centerWidth;

    ImGui::SetNextWindowPos(ImVec2(leftPanelWidth, menuBarHeight));

    ImGui::SetNextWindowSize(ImVec2(sceneWidth, centerHeight));

    if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))

    {

        gizmoSystem->drawSceneView(scene, assetManager, offscreenDescriptorSet, ImGui::GetWindowPos(), ImGui::GetWindowSize(), mode);

    }

    ImGui::End();



    // 4. Game View

    if (gizmoSystem->showGameViewWindow || gizmoSystem->isGameFullscreen)

    {

        if (gizmoSystem->isGameViewDetached)

        {

            ImGui::SetNextWindowSize(ImVec2(680, 480), ImGuiCond_FirstUseEver);

            if (ImGui::Begin("Game View (Pop-out Window)", &gizmoSystem->showGameViewWindow, ImGuiWindowFlags_NoScrollbar))

            {

                if (ImGui::Button(" Gộp Lại Vào Editor "))

                {

                    gizmoSystem->isGameViewDetached = false;

                }

                ImGui::SameLine();

                ImGui::TextDisabled("| Kéo di chuyển / thay đổi kích thước cửa sổ tự do");

                ImGui::Separator();



                gizmoSystem->drawGameView(gameViewDescriptorSet, ImGui::GetWindowPos(), ImGui::GetWindowSize(), gameScore, highScore, onToggleFullscreen, mode, scene, renderer);

            }

            ImGui::End();

        }

        else

        {

            ImGui::SetNextWindowPos(ImVec2(leftPanelWidth + centerWidth * 0.5f, menuBarHeight));

            ImGui::SetNextWindowSize(ImVec2(centerWidth * 0.5f, centerHeight));

            if (ImGui::Begin("Game", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar))

            {

                gizmoSystem->drawGameView(gameViewDescriptorSet, ImGui::GetWindowPos(), ImGui::GetWindowSize(), gameScore, highScore, onToggleFullscreen, mode, scene, renderer);

            }

            ImGui::End();

        }

    }



    // 5. Inspector Panel

    drawInspectorPanel(scene, assetManager, scriptEngine, renderer, gizmoSystem, mode, menuBarHeight, centerHeight, editorWidth);



    // 6. Console / Status Bar

    drawConsolePanel(selectedGpuName, mode, scene, editorHeight, windowWidth);



    // 7. Visual Profiler Panel

    profilerPanel->draw();



    // 8. Asset Browser Panel

    assetBrowserPanel->draw(assetManager, scene, editorWidth, bottomPanelHeight);

    if (!assetBrowserPanel->getVisible())

    {

        ImGui::SetNextWindowPos(ImVec2(10.0f, editorHeight - 48.0f), ImGuiCond_Always);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.14f, 0.18f, 0.85f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.96f, 0.65f, 0.14f, 0.9f));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

        if (ImGui::Begin("##CollapsedAssetTab", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))

        {

            if (ImGui::Button(" 🎯 TARGETS & ASSETS 🔼 "))

            {

                assetBrowserPanel->setVisible(true);

            }

        }

        ImGui::End();

        ImGui::PopStyleColor(3);

    }



    // 9. Scene Generator Panel

    sceneGenerator->drawSceneGeneratorPanel(scene, assetManager, gizmoSystem);



    // 10. Model Downloader Panel

    modelDownloader->draw(scene, assetManager);



    // 11. Modal Dialogs (Load Scene & Save Scene)

    drawLoadSceneModal(scene, assetManager, gizmoSystem, renderer, physEngine, scriptEngine, mode);

    drawSaveSceneModal(scene, assetManager, gizmoSystem, renderer);



    ImGui::Render();

}



void EditorUI::drawMainMenuBar(Scene* scene, AppMode& mode, int& gameScore, const std::string& selectedGpuName,

                               std::function<void()> onPlay, std::function<void()> onStop,

                               std::function<void()> onReloadLua, std::function<void()> onResetScene,

                               std::function<void()> onSaveScene, std::function<void()> onLoadScene,

                               GizmoSystem* gizmoSystem, ProfilerPanel* profilerPanel,

                               AssetBrowserPanel* assetBrowserPanel, SceneGenerator* sceneGenerator,

                               ModelDownloader* modelDownloader, VulkanRenderer* renderer,

                               AssetManager* assetManager)

{

    if (ImGui::BeginMainMenuBar())

    {

        ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.0f, 1.0f), "  ANTIGRAVITY ENGINE");

        ImGui::Separator();



        if (ImGui::BeginMenu("File"))

        {

            if (ImGui::MenuItem("New Scene"))

            {

                if (onResetScene) onResetScene();

                gameScore = 0;

            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S"))

            {

                if (onSaveScene) onSaveScene();

            }

            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))

            {

                showSaveSceneModal = true;

            }

            if (ImGui::MenuItem("Load Scene...", "Ctrl+L"))

            {

                showLoadSceneModal = true;

            }

            if (ImGui::MenuItem("🐵 Export for jMonkeyEngine (.json)...", "Ctrl+E"))

            {

                const char* filterPatterns[] = { "*.json" };

                std::string defaultExportPath = "../Jme_scene/scene.json";

                if (!std::filesystem::exists("../Jme_scene") && std::filesystem::exists("D:/Game_prj/Jme_scene"))

                {

                    defaultExportPath = "D:/Game_prj/Jme_scene/scene.json";

                }

                const char* savePath = tinyfd_saveFileDialog(

                    "Xuat Scene cho jMonkeyEngine (.json)",

                    defaultExportPath.c_str(),

                    1,

                    filterPatterns,

                    "JSON Scene File (*.json)"

                );

                std::string targetFile = savePath ? savePath : "scene_jme.json";

                if (scene)

                {

                    scene->exportToJMonkeyEngine(targetFile, assetManager, gizmoSystem, renderer);

                    if (gizmoSystem)

                    {

                        gizmoSystem->lastExportedImagePath = targetFile;

                        gizmoSystem->exportNotificationTimer = 6.0f;

                    }

                }

            }

            if (ImGui::MenuItem(mode == AppMode::PLAY ? "⏹ Stop jMonkeyEngine" : "▶ Play in jMonkeyEngine", "F5"))

            {

                if (mode == AppMode::PLAY)

                {

                    if (onStop) onStop();

                }

                else

                {

                    if (onPlay) onPlay();

                }

            }

            ImGui::Separator();

            if (ImGui::MenuItem("📷 Xuất Ảnh Khung Cam (PNG/JPG)", "F12"))

            {

                if (renderer && gizmoSystem)

                {

                    bool is2m4 = (gizmoSystem->gameAspectRatioMode == GameViewAspectRatioMode::PHOTO_2M4_X_2M);

                    uint32_t expW = is2m4 ? 2400 : 1920;

                    uint32_t expH = is2m4 ? 2000 : 1080;

                    const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };

                    std::string defaultName = is2m4 ? "Cam_View_2m4x2m.png" : "Cam_View.png";

                    const char* savePath = tinyfd_saveFileDialog(

                        "Lưu Ảnh Khung Cam (Export Camera View Image)",

                        defaultName.c_str(),

                        4,

                        filterPatterns,

                        "Image Files (*.png, *.jpg, *.bmp)"

                    );

                    if (savePath)

                    {

                        if (renderer->exportGameViewImage(savePath, expW, expH))

                        {

                            gizmoSystem->lastExportedImagePath = savePath;

                            gizmoSystem->exportNotificationTimer = 7.0f;

                        }

                    }

                }

            }

            ImGui::EndMenu();

        }

        ImGui::Separator();



        if (mode == AppMode::PLAY)

        {

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.65f, 0.32f, 1.0f));

            if (ImGui::Button(" [ 🐵 PLAYING (jME) ] "))

            {

                if (onStop) onStop();

            }

            ImGui::PopStyleColor();

            if (ImGui::IsItemHovered())

            {

                ImGui::SetTooltip("jMonkeyEngine đang chạy. Nhấn để dừng (F5)");

            }



            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.20f, 0.20f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));

            if (ImGui::Button(" [ ⏹ STOP jME ] "))

            {

                if (onStop) onStop();

            }

            ImGui::PopStyleColor(2);

            if (ImGui::IsItemHovered())

            {

                ImGui::SetTooltip("Dừng ứng dụng jMonkeyEngine và trở về chế độ chỉnh sửa Edit Mode (F5)");

            }



            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.1f, 1.0f));

            if (ImGui::Button(" [ RELOAD LUA ] "))

            {

                if (onReloadLua) onReloadLua();

                std::cout << "Lua scripts hot-reloaded successfully during PLAY mode!\n";

            }

            ImGui::PopStyleColor();

        }

        else

        {

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.62f, 0.32f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.75f, 0.38f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.50f, 0.25f, 1.0f));

            if (ImGui::Button(" [ ▶ PLAY (jMonkeyEngine) ] "))

            {

                if (onPlay) onPlay();

            }

            ImGui::PopStyleColor(3);

            if (ImGui::IsItemHovered())

            {

                ImGui::SetTooltip("Xuất cảnh hiện tại sang jMonkeyEngine và khởi chạy trực tiếp (F5)");

            }

        }



        ImGui::Separator();

        if (ImGui::Button(gizmoSystem->showGameViewWindow ? " [ [x] Game View ] " : " [ [ ] Game View ] "))

        {

            gizmoSystem->showGameViewWindow = !gizmoSystem->showGameViewWindow;

        }

        if (gizmoSystem->showGameViewWindow)

        {

            ImGui::SameLine();

            if (ImGui::Button(gizmoSystem->isGameViewDetached ? " [ Gộp Vào Editor ] " : " [ Pop-out Cửa Sổ Nổi ] "))

            {

                gizmoSystem->isGameViewDetached = !gizmoSystem->isGameViewDetached;

            }

        }



        ImGui::Separator();

        if (ImGui::Button(profilerPanel->getVisible() ? " [ [x] Visual Profiler ] " : " [ [ ] Visual Profiler ] "))

        {

            profilerPanel->getVisible() = !profilerPanel->getVisible();

        }



        ImGui::Separator();

        if (ImGui::Button(assetBrowserPanel->getVisible() ? " [ [x] 🎯 Targets & Assets ] " : " [ [ ] 🎯 Targets & Assets ] "))

        {

            assetBrowserPanel->getVisible() = !assetBrowserPanel->getVisible();

        }



        ImGui::Separator();

        if (ImGui::Button(sceneGenerator->getVisible() ? " [ [x] Sinh Cảnh 3D ] " : " [ [ ] Sinh Cảnh 3D ] "))

        {

            sceneGenerator->getVisible() = !sceneGenerator->getVisible();

        }



        ImGui::Separator();

        if (ImGui::Button(modelDownloader->getVisible() ? " [ [x] Online Downloader ] " : " [ [ ] Online Downloader ] "))

        {

            modelDownloader->getVisible() = !modelDownloader->getVisible();

        }



        ImGui::Separator();

        ImGui::Text("GPU: %s", selectedGpuName.c_str());



        ImGui::Separator();

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);



        ImGui::EndMainMenuBar();

    }

}



void EditorUI::drawHierarchyPanel(Scene* scene, AssetManager* assetManager, ScriptEngine* scriptEngine,

                                  GizmoSystem* gizmoSystem, AppMode mode, SceneGenerator* sceneGenerator,

                                  float menuBarHeight, float centerHeight)

{

    auto& sceneObjects = scene->getObjects();

    int selectedObjectIndex = scene->getSelectedObjectIndex();



    ImGui::SetNextWindowPos(ImVec2(0.0f, menuBarHeight), ImGuiCond_Always);

    ImGui::SetNextWindowSize(ImVec2(leftPanelWidth, centerHeight), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))

    {

        leftPanelWidth = ImGui::GetWindowWidth();

        if (ImGui::Button(" + Create ", ImVec2(-1, 25)))

        {

            ImGui::OpenPopup("CreateObjectPopup");

        }

        

        if (ImGui::BeginPopup("CreateObjectPopup"))

        {

            if (ImGui::MenuItem("3D Cube"))

            {

                scene->saveHistory();

                SceneObject newObj;

                newObj.id = (int)sceneObjects.size();

                newObj.name = "Cube " + std::to_string(sceneObjects.size());

                newObj.type = ObjectType::CUBE;

                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);

                newObj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

                newObj.scale = glm::vec3(0.5f, 0.5f, 0.5f);

                newObj.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

                newObj.isPhysicsEnabled = false;

                newObj.meshId = assetManager->getCubeMeshId();

                newObj.syncComponents();

                sceneObjects.push_back(newObj);

                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

            }

            if (ImGui::MenuItem("3D Sphere"))

            {

                scene->saveHistory();

                SceneObject newObj;

                newObj.id = (int)sceneObjects.size();

                newObj.name = "Sphere " + std::to_string(sceneObjects.size());

                newObj.type = ObjectType::SPHERE;

                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);

                newObj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

                newObj.scale = glm::vec3(0.5f, 0.5f, 0.5f);

                newObj.color = glm::vec4(1.0f, 0.5f, 0.5f, 1.0f);

                newObj.isPhysicsEnabled = false;

                newObj.meshId = assetManager->getSphereMeshId();

                newObj.syncComponents();

                sceneObjects.push_back(newObj);

                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

            }

            if (ImGui::MenuItem("3D Cylinder"))

            {

                scene->saveHistory();

                SceneObject newObj;

                newObj.id = (int)sceneObjects.size();

                newObj.name = "Cylinder " + std::to_string(sceneObjects.size());

                newObj.type = ObjectType::CUBE;

                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);

                newObj.scale = glm::vec3(0.5f, 1.0f, 0.5f);

                newObj.color = glm::vec4(0.7f, 0.7f, 0.8f, 1.0f);

                newObj.meshId = assetManager->getCylinderMeshId();

                newObj.syncComponents();

                sceneObjects.push_back(newObj);

                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

            }

            if (ImGui::MenuItem("3D Cone"))

            {

                scene->saveHistory();

                SceneObject newObj;

                newObj.id = (int)sceneObjects.size();

                newObj.name = "Cone " + std::to_string(sceneObjects.size());

                newObj.type = ObjectType::CUBE;

                newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);

                newObj.scale = glm::vec3(0.6f, 1.0f, 0.6f);

                newObj.color = glm::vec4(0.2f, 0.7f, 0.3f, 1.0f);

                newObj.meshId = assetManager->getConeMeshId();

                newObj.syncComponents();

                sceneObjects.push_back(newObj);

                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

            }

            if (ImGui::MenuItem("Directional Light"))

            {

                scene->saveHistory();

                SceneObject newObj;

                newObj.id = (int)sceneObjects.size();

                newObj.name = "Light " + std::to_string(sceneObjects.size());

                newObj.type = ObjectType::LIGHT;

                newObj.position = glm::vec3(0.0f, 2.0f, 0.0f);

                newObj.rotation = glm::vec3(45.0f, 45.0f, 0.0f);

                newObj.scale = glm::vec3(0.3f, 0.3f, 0.3f);

                newObj.color = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);

                newObj.isPhysicsEnabled = false;

                newObj.meshId = assetManager->getCubeMeshId();

                newObj.syncComponents();

                sceneObjects.push_back(newObj);

                scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

            }

            if (ImGui::MenuItem("3D Model (.glb, .gltf, .obj, .fbx)..."))

            {

                const char* filterPatterns[4] = { "*.glb", "*.gltf", "*.obj", "*.fbx" };

                const char* filePath = tinyfd_openFileDialog("Thêm 3D Model (Tỉ lệ 1:1 Metric)", "", 4, filterPatterns, "3D Models", 0);

                if (filePath)

                {

                    try {

                        int meshId = assetManager->load3DModelAsset(filePath);

                        if (meshId >= 0)

                        {

                            scene->saveHistory();

                            SceneObject newObj;

                            newObj.id = (int)sceneObjects.size();

                            std::filesystem::path p(filePath);

                            newObj.name = p.stem().string();

                            newObj.type = ObjectType::CUBE;

                            newObj.position = glm::vec3(0.0f, 0.0f, 0.0f);

                            newObj.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

                            newObj.scale = glm::vec3(1.0f, 1.0f, 1.0f); // 1:1 Metric Scale!

                            newObj.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

                            newObj.isPhysicsEnabled = false;

                            newObj.meshId = meshId;

                            newObj.modelPath = filePath;

                            const auto& meshes = assetManager->getMeshes();

                            if (meshId < static_cast<int>(meshes.size()) && meshes[meshId].defaultTextureId >= 0) {

                                newObj.textureId = meshes[meshId].defaultTextureId;

                                newObj.roughness = meshes[meshId].defaultRoughness;

                                newObj.metallic = meshes[meshId].defaultMetallic;

                            }

                            newObj.syncComponents();

                            newObj.autoAttachAnimatorIfSkinned(meshes);

                            sceneObjects.push_back(newObj);

                            scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

                        }

                    }

                    catch (const std::exception& e) {

                        std::string msg = e.what();

                        if (msg.rfind("ANIMATION_ONLY_FBX:", 0) == 0)

                        {

                            auto selectedObj = scene->getSelectedObject();

                            auto animComp = selectedObj ? selectedObj->getAnimator() : nullptr;

                            auto clip = AnimationLoader::loadFBXClip(filePath, animComp ? animComp->skeleton.get() : nullptr);

                            if (clip && animComp)

                            {

                                animComp->addClip(clip);

                                animComp->play(clip->name, true);

                                tinyfd_messageBox("Animation Imported", ("Đã nạp thành công Animation Clip '" + clip->name + "' và gán vào nhân vật '" + selectedObj->name + "'!").c_str(), "ok", "info", 1);

                            }

                            else if (clip)

                            {

                                tinyfd_messageBox("Animation Clip Detected", ("Tệp '" + std::filesystem::path(filePath).filename().string() + "' là một Animation Clip (chứa motion/bộ xương, không chứa 3D mesh hình thể).\n\nĐể sử dụng: Hãy nạp nhân vật trước (ví dụ tm_classy_black_guy.fbx), sau đó bấm 'Import Mixamo FBX Animation' trong Animator Component!").c_str(), "ok", "info", 1);

                            }

                        }

                        else

                        {

                            tinyfd_messageBox("Lỗi nạp model", e.what(), "ok", "error", 1);

                        }

                    }

                }

            }

            if (ImGui::MenuItem("🎬 Animation Clip (.fbx)..."))

            {

                const char* filterPatterns[1] = { "*.fbx" };

                const char* filePath = tinyfd_openFileDialog("Nạp Animation FBX", "assets/animations/", 1, filterPatterns, "FBX Animation Clip", 0);

                if (filePath)

                {

                    auto selectedObj = scene->getSelectedObject();

                    auto animComp = selectedObj ? selectedObj->getAnimator() : nullptr;

                    auto clip = AnimationLoader::loadFBXClip(filePath, animComp ? animComp->skeleton.get() : nullptr);

                    if (clip)

                    {

                        if (animComp)

                        {

                            animComp->addClip(clip);

                            animComp->play(clip->name, true);

                            tinyfd_messageBox("Nạp Animation thành công", ("Đã gán Animation '" + clip->name + "' vào '" + selectedObj->name + "' và đang phát!").c_str(), "ok", "info", 1);

                        }

                        else

                        {

                            tinyfd_messageBox("Animation Clip Đã Nạp", ("Đã đọc thành công Animation Clip '" + clip->name + "' (Thời lượng: " + std::to_string(clip->duration) + "s, " + std::to_string(clip->tracks.size()) + " tracks).\n\nHãy chọn một nhân vật có Animator (hoặc thêm Animator Component) rồi nạp lại!").c_str(), "ok", "info", 1);

                        }

                    }

                    else

                    {

                        tinyfd_messageBox("Lỗi Animation", "Không thể trích xuất animation từ tệp FBX này.", "ok", "error", 1);

                    }

                }

            }

            if (ImGui::MenuItem("📦 Import Asset Pack (.zip)..."))

            {

                const char* filterPatterns[1] = { "*.zip" };

                const char* zipPath = tinyfd_openFileDialog("Chọn Asset Pack (.zip)", "", 1, filterPatterns, "ZIP Archive (*.zip)", 0);

                if (zipPath)

                {

                    importZipAssetPack(zipPath, assetManager, scene);

                }

            }

            ImGui::Separator();

            if (ImGui::MenuItem("Sinh Cảnh 3D Tự Động..."))

            {

                sceneGenerator->getVisible() = true;

                sceneGenerator->generate3DScene(scene, assetManager, gizmoSystem);

            }

            ImGui::EndPopup();

        }



        ImGui::Spacing();

        ImGui::Separator();

        ImGui::Spacing();



        if (ImGui::TreeNodeEx("SampleScene", ImGuiTreeNodeFlags_DefaultOpen))

        {

            for (size_t i = 0; i < sceneObjects.size(); ++i)

            {

                ImGui::PushID(static_cast<int>(i));



                std::string icon = "🧊 ";

                if (sceneObjects[i].type == ObjectType::SPHERE) icon = "🟡 ";

                if (sceneObjects[i].type == ObjectType::PLANE) icon = "🟩 ";

                if (sceneObjects[i].type == ObjectType::LIGHT) icon = "💡 ";

                

                std::string label = icon + sceneObjects[i].name;

                

                if (ImGui::Selectable(label.c_str(), selectedObjectIndex == static_cast<int>(i)))

                {

                    scene->setSelectedObjectIndex(static_cast<int>(i));

                }



                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))

                {

                    scene->setSelectedObjectIndex(static_cast<int>(i));

                    if (gizmoSystem)

                    {

                        gizmoSystem->focusOnObject(static_cast<int>(i), *scene);

                    }

                }



                if (ImGui::BeginDragDropTarget())

                {

                    if (const ImGuiPayload* payloadLua = ImGui::AcceptDragDropPayload("DND_ASSET_LUA"))

                    {

                        const char* assetPath = static_cast<const char*>(payloadLua->Data);

                        std::ifstream t(assetPath);

                        if (t.is_open())

                        {

                            std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

                            sceneObjects[i].luaScripts.push_back(scriptContent);

                            sceneObjects[i].syncComponents();

                            scene->setSelectedObjectIndex(static_cast<int>(i));

                            if (mode == AppMode::PLAY && scriptEngine) scriptEngine->reloadLuaScripts(scene);

                        }

                    }

                    else if (const ImGuiPayload* payloadTex = ImGui::AcceptDragDropPayload("DND_ASSET_TEXTURE"))

                    {

                        const char* assetPath = static_cast<const char*>(payloadTex->Data);

                        int texId = assetManager->loadTextureAsset(assetPath);

                        if (texId >= 0)

                        {

                            sceneObjects[i].textureId = texId;

                            sceneObjects[i].syncComponents();

                            scene->setSelectedObjectIndex(static_cast<int>(i));

                        }

                    }

                    else if (const ImGuiPayload* payloadModel = ImGui::AcceptDragDropPayload("DND_ASSET_MODEL"))

                    {

                        const char* assetPath = static_cast<const char*>(payloadModel->Data);

                        int meshId = assetManager->load3DModelAsset(assetPath);

                        if (meshId >= 0)

                        {

                            sceneObjects[i].meshId = meshId;

                            sceneObjects[i].modelPath = assetPath;

                            const auto& meshes = assetManager->getMeshes();

                            if (meshId < static_cast<int>(meshes.size()) && meshes[meshId].defaultTextureId >= 0) {

                                sceneObjects[i].textureId = meshes[meshId].defaultTextureId;

                                sceneObjects[i].roughness = meshes[meshId].defaultRoughness;

                                sceneObjects[i].metallic = meshes[meshId].defaultMetallic;

                            }

                            sceneObjects[i].syncComponents();

                            sceneObjects[i].autoAttachAnimatorIfSkinned(meshes);

                            scene->setSelectedObjectIndex(static_cast<int>(i));

                        }

                    }

                    else if (const ImGuiPayload* payloadAnim = ImGui::AcceptDragDropPayload("DND_ASSET_ANIM"))

                    {

                        const char* animPath = static_cast<const char*>(payloadAnim->Data);

                        sceneObjects[i].autoAttachAnimatorIfSkinned(assetManager->getMeshes());

                        auto animComp = sceneObjects[i].getAnimator();

                        if (animComp)

                        {

                            auto clip = AnimationLoader::loadFBXClip(animPath, animComp->skeleton.get());

                            if (clip)

                            {

                                animComp->addClip(clip);

                                animComp->play(clip->name, true);

                            }

                        }

                        scene->setSelectedObjectIndex(static_cast<int>(i));

                    }

                    ImGui::EndDragDropTarget();

                }



                if (ImGui::BeginPopupContextItem())

                {

                    scene->setSelectedObjectIndex(static_cast<int>(i));

                    ImGui::TextDisabled("Entity: %s", sceneObjects[i].name.c_str());

                    ImGui::Separator();



                    if (ImGui::MenuItem("🎯 Focus Camera (F)"))

                    {

                        if (gizmoSystem)

                        {

                            gizmoSystem->focusOnObject(static_cast<int>(i), *scene);

                        }

                    }

                    

                    if (ImGui::MenuItem("Duplicate Entity"))

                    {

                        scene->saveHistory();

                        SceneObject dup = sceneObjects[i];

                        dup.id = (int)sceneObjects.size();

                        dup.name = sceneObjects[i].name + " (Copy)";

                        dup.position += glm::vec3(0.5f, 0.0f, 0.5f);

                        dup.syncComponents();

                        sceneObjects.push_back(dup);

                        scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);

                    }



                    if (ImGui::MenuItem("Delete Entity"))

                    {

                        scene->saveHistory();

                        sceneObjects.erase(sceneObjects.begin() + i);

                        scene->setSelectedObjectIndex(-1);

                        ImGui::EndPopup();

                        ImGui::PopID();

                        break;

                    }

                    ImGui::EndPopup();

                }



                ImGui::PopID();

            }



            ImGui::Spacing();

            ImGui::Separator();

            ImGui::Spacing();



            if (ImGui::Selectable("Main Camera", selectedObjectIndex == -1))

            {

                scene->setSelectedObjectIndex(-1);

            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))

            {

                scene->setSelectedObjectIndex(-1);

                if (gizmoSystem)

                {

                    gizmoSystem->focusOnObject(-1, *scene);

                }

            }



            ImGui::TreePop();

        }

    }

    ImGui::End();

}



void EditorUI::drawInspectorPanel(Scene* scene, AssetManager* assetManager, ScriptEngine* scriptEngine,

                                  VulkanRenderer* renderer, GizmoSystem* gizmoSystem, AppMode mode,

                                  float menuBarHeight, float centerHeight, float editorWidth)

{

    auto& sceneObjects = scene->getObjects();

    int selectedObjectIndex = scene->getSelectedObjectIndex();



    ImGui::SetNextWindowPos(ImVec2(editorWidth - rightPanelWidth, menuBarHeight), ImGuiCond_Always);

    ImGui::SetNextWindowSize(ImVec2(rightPanelWidth, centerHeight), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))

    {

        rightPanelWidth = ImGui::GetWindowWidth();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));

        

        static char nameBuf[64] = "";

        static char tagBuf[64] = "Untagged";

        static bool isStatic = false;



        if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))

        {

            auto& obj = sceneObjects[selectedObjectIndex];

            obj.syncComponents();

            

            ImGui::Text("Entity (GameObject): ");

            ImGui::SameLine();

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "%s (ID: %d)", obj.name.c_str(), obj.id);

            ImGui::Separator();



            if (ImGui::Button(" 🎯 Tìm & Focus Camera Đến Model Này (Phím F) ", ImVec2(-1, 26)))

            {

                if (gizmoSystem)

                {

                    gizmoSystem->focusOnObject(selectedObjectIndex, *scene);

                }

            }



            snprintf(nameBuf, sizeof(nameBuf), "%s", obj.name.c_str());

            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))

            {

                obj.name = nameBuf;

            }



            ImGui::Checkbox("Static", &isStatic);

            ImGui::SameLine();

            ImGui::TextDisabled("|");

            ImGui::SameLine();

            ImGui::Text("Tag:");

            ImGui::SameLine();

            ImGui::SetNextItemWidth(90);

            ImGui::InputText("##Tag", tagBuf, sizeof(tagBuf));



            ImGui::Spacing();

            ImGui::Separator();

            ImGui::Spacing();



            // 1. Transform Component

            if (ImGui::CollapsingHeader("Transform Component", ImGuiTreeNodeFlags_DefaultOpen))

            {

                ImGui::DragFloat3("Position (X,Y,Z)", &obj.position.x, 0.05f);

                ImGui::DragFloat3("Rotation (X,Y,Z)", &obj.rotation.x, 0.5f);

                ImGui::DragFloat3("Scale (X,Y,Z)", &obj.scale.x, 0.02f, 0.01f, 10.0f);

            }

            ImGui::Spacing();



            // 1.1 Parent & Bone Attachment (Socket System)

            if (ImGui::CollapsingHeader("🔗 Liên Kết Xương (Bone Socket)", ImGuiTreeNodeFlags_DefaultOpen))

            {

                std::string currentParentName = "None (Không có - Root)";

                if (obj.parentId >= 0 && obj.parentId < static_cast<int>(sceneObjects.size()) && obj.parentId != selectedObjectIndex)

                {

                    currentParentName = sceneObjects[obj.parentId].name + " (ID: " + std::to_string(obj.parentId) + ")";

                }



                if (ImGui::BeginCombo("Đối Tượng Cha (Parent)", currentParentName.c_str()))

                {

                    if (ImGui::Selectable("None (Không có - Độc lập)", obj.parentId < 0))

                    {

                        obj.parentId = -1;

                        obj.parentBoneName = "";

                    }

                    for (size_t p = 0; p < sceneObjects.size(); ++p)

                    {

                        if (static_cast<int>(p) == selectedObjectIndex) continue;

                        std::string pLabel = sceneObjects[p].name + " (ID: " + std::to_string(p) + ")";

                        bool isSel = (obj.parentId == static_cast<int>(p));

                        if (ImGui::Selectable(pLabel.c_str(), isSel))

                        {

                            obj.parentId = static_cast<int>(p);

                        }

                    }

                    ImGui::EndCombo();

                }



                // Smart 1-Click Quick Attach if unparented weapon

                if (obj.parentId < 0)
                {
                    int autoCharIdx = -1;
                    for (size_t p = 0; p < sceneObjects.size(); ++p)
                    {
                        if (static_cast<int>(p) == selectedObjectIndex) continue;
                        auto anim = sceneObjects[p].getAnimator();
                        if (anim && anim->skeleton)
                        {
                            autoCharIdx = static_cast<int>(p);
                            break;
                        }
                    }

                    if (autoCharIdx >= 0)
                    {
                        std::string btnLabel = "⚡ Khóa Khớp Socket Vũ Khí (Gắn Vào [" + sceneObjects[autoCharIdx].name + "] & Khóa 2 Tay Chuẩn Tác Chiến)";
                        if (ImGui::Button(btnLabel.c_str(), ImVec2(-1, 28)))
                        {
                            obj.parentId = autoCharIdx;
                            auto anim = sceneObjects[autoCharIdx].getAnimator();
                            if (anim && anim->skeleton)
                            {
                                int rIdx = anim->skeleton->findBoneIndex("Socket_Weapon_R");
                                if (rIdx < 0) rIdx = anim->skeleton->findBoneIndex("CC_Base_R_Hand");
                                if (rIdx < 0) rIdx = anim->skeleton->findBoneIndex("RightHand");
                                if (rIdx < 0) rIdx = anim->skeleton->findBoneIndex("mixamorig:RightHand");
                                if (rIdx < 0) rIdx = anim->skeleton->findBoneIndex("ValveBiped.Bip01_R_Hand");
                                if (rIdx < 0) rIdx = anim->skeleton->findBoneIndex("hand_r");
                                if (rIdx < 0) rIdx = anim->skeleton->findBoneIndex("weapon_r");
                                obj.parentBoneName = (rIdx >= 0) ? anim->skeleton->bones[rIdx].name : "CC_Base_R_Hand";

                                int lIdx = anim->skeleton->findBoneIndex("CC_Base_L_Hand");
                                if (lIdx < 0) lIdx = anim->skeleton->findBoneIndex("LeftHand");
                                if (lIdx < 0) lIdx = anim->skeleton->findBoneIndex("mixamorig:LeftHand");
                                if (lIdx < 0) lIdx = anim->skeleton->findBoneIndex("ValveBiped.Bip01_L_Hand");
                                if (lIdx < 0) lIdx = anim->skeleton->findBoneIndex("hand_l");
                                if (lIdx < 0) lIdx = anim->skeleton->findBoneIndex("weapon_l");
                                obj.secondaryBoneName = (lIdx >= 0) ? anim->skeleton->bones[lIdx].name : "CC_Base_L_Hand";

                                obj.enableTwoHandedAiming = (lIdx >= 0);
                                obj.enableDynamicWeaponSockets = true;
                                obj.position = glm::vec3(0.0f);
                                obj.rotation = glm::vec3(0.0f);
                                obj.scale = glm::vec3(1.0f);
                            }
                        }
                    }
                }

                if (obj.parentId >= 0 && obj.parentId < static_cast<int>(sceneObjects.size()))

                {

                    auto& parentObj = sceneObjects[obj.parentId];

                    auto parentAnim = parentObj.getAnimator();



                    if (parentAnim && parentAnim->skeleton)

                    {

                        const auto& skel = parentAnim->skeleton;

                        std::string currentBone = obj.parentBoneName.empty() ? "(Root của Cha - Không theo xương)" : obj.parentBoneName;

                        if (ImGui::BeginCombo("Tay Phải / Báng Súng (Primary)", currentBone.c_str()))

                        {

                            if (ImGui::Selectable("(Root của Cha - Không theo xương)", obj.parentBoneName.empty()))

                            {

                                obj.parentBoneName = "";

                            }

                            for (size_t b = 0; b < skel->bones.size(); ++b)

                            {

                                const auto& bName = skel->bones[b].name;

                                bool isBSelected = (obj.parentBoneName == bName);

                                if (ImGui::Selectable(bName.c_str(), isBSelected))

                                {

                                    obj.parentBoneName = bName;

                                }

                            }

                            ImGui::EndCombo();

                        }



                        // Dynamic Socket Checkbox

                        ImGui::Checkbox("🎯 Socket Thông Minh (Reload: Tay Phải, Grenade: Đeo Ngực)", &obj.enableDynamicWeaponSockets);

                        if (obj.enableDynamicWeaponSockets)

                        {

                            ImGui::SameLine();

                            ImGui::TextDisabled("(?)");

                            if (ImGui::IsItemHovered())

                                ImGui::SetTooltip("Tự động chuyển đổi vị trí gắn súng theo animation:\n- Reload: Giữ chắc tay phải, ngắt khóa 2 tay\n- Toss Grenade: Đeo chéo trước ngực để tự do ném rồi chạy\n- Aim/Shoot/Run: Cầm tác chiến 2 tay");

                        }



                        // Two-handed lock checkbox

                        ImGui::Checkbox("☑ Khóa 2 Tay (Khớp Nòng Súng Vào Tay Trái)", &obj.enableTwoHandedAiming);



                        if (obj.enableTwoHandedAiming)

                        {

                            std::string secBone = obj.secondaryBoneName.empty() ? "(Chưa chọn tay trái)" : obj.secondaryBoneName;

                            if (ImGui::BeginCombo("Tay Trái / Ốp Tay & Họng (Secondary)", secBone.c_str()))

                            {

                                for (size_t b = 0; b < skel->bones.size(); ++b)

                                {

                                    const auto& bName = skel->bones[b].name;

                                    bool isSecSel = (obj.secondaryBoneName == bName);

                                    if (ImGui::Selectable(bName.c_str(), isSecSel))

                                    {

                                        obj.secondaryBoneName = bName;

                                    }

                                }

                                ImGui::EndCombo();

                            }

                        }



                        ImGui::Spacing();

                        // 1. Primary Action: Instant 2-handed tactical grip lock
                        if (ImGui::Button("⚡ Khóa Khớp Socket Vũ Khí (Khóa 2 Tay Chuẩn Tác Chiến)", ImVec2(-1, 28)))
                        {
                            int rIdx = skel->findBoneIndex("Socket_Weapon_R");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("CC_Base_R_Hand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("RightHand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("mixamorig:RightHand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("ValveBiped.Bip01_R_Hand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("hand_r");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("weapon_r");
                            obj.parentBoneName = (rIdx >= 0) ? skel->bones[rIdx].name : "CC_Base_R_Hand";

                            int lIdx = skel->findBoneIndex("CC_Base_L_Hand");
                            if (lIdx < 0) lIdx = skel->findBoneIndex("LeftHand");
                            if (lIdx < 0) lIdx = skel->findBoneIndex("mixamorig:LeftHand");
                            if (lIdx < 0) lIdx = skel->findBoneIndex("ValveBiped.Bip01_L_Hand");
                            if (lIdx < 0) lIdx = skel->findBoneIndex("hand_l");
                            if (lIdx < 0) lIdx = skel->findBoneIndex("weapon_l");
                            obj.secondaryBoneName = (lIdx >= 0) ? skel->bones[lIdx].name : "CC_Base_L_Hand";

                            obj.enableTwoHandedAiming = (lIdx >= 0);
                            obj.enableDynamicWeaponSockets = true;
                            obj.position = glm::vec3(0.0f);
                            obj.rotation = glm::vec3(0.0f);
                            obj.scale = glm::vec3(1.0f);
                        }

                        // 2. Chest Sling
                        if (ImGui::Button("🎒 Đeo Súng Trước Ngực (Chest Slung)", ImVec2(-1, 26)))
                        {
                            int cIdx = skel->findBoneIndex("CC_Base_Spine02");
                            if (cIdx < 0) cIdx = skel->findBoneIndex("Spine2");
                            if (cIdx < 0) cIdx = skel->findBoneIndex("mixamorig:Spine2");
                            if (cIdx < 0) cIdx = skel->findBoneIndex("ValveBiped.Bip01_Spine2");
                            obj.parentBoneName = (cIdx >= 0) ? skel->bones[cIdx].name : "CC_Base_Spine02";
                            obj.enableTwoHandedAiming = false;
                            obj.position = obj.chestOffsetPosition;
                            obj.rotation = obj.chestOffsetRotation;
                            obj.scale = glm::vec3(1.0f);
                        }

                        // 3. Independent Single Right Hand
                        if (ImGui::Button("🔫 Gắn Tay Phải Độc Lập (Chỉ 1 Tay R_Hand)", ImVec2(-1, 24)))
                        {
                            int rIdx = skel->findBoneIndex("Socket_Weapon_R");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("CC_Base_R_Hand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("RightHand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("mixamorig:RightHand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("ValveBiped.Bip01_R_Hand");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("hand_r");
                            if (rIdx < 0) rIdx = skel->findBoneIndex("weapon_r");
                            obj.parentBoneName = (rIdx >= 0) ? skel->bones[rIdx].name : "CC_Base_R_Hand";
                            obj.secondaryBoneName = "";
                            obj.enableTwoHandedAiming = false;
                            obj.position = glm::vec3(0.0f);
                            obj.rotation = glm::vec3(0.0f);
                            obj.scale = glm::vec3(1.0f);
                        }
                    }
                    else

                    {

                        ImGui::TextDisabled("Đối tượng cha không có khung xương / Animator.");

                    }

                }

            }

            ImGui::Spacing();



            // 2. Mesh Renderer Component

            if (obj.hasComponent(ComponentType::MESH_RENDERER))

            {

                if (ImGui::CollapsingHeader("Mesh Renderer Component", ImGuiTreeNodeFlags_DefaultOpen))

                {

                    ImGui::ColorEdit4("Mesh Color", &obj.color.x);

                    

                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "PBR Material Parameters");

                    ImGui::Checkbox("Enable PBR Shading", &obj.usePBR);

                    if (obj.usePBR)

                    {

                        ImGui::SliderFloat("Roughness", &obj.roughness, 0.0f, 1.0f, "%.2f (0=Smooth, 1=Rough)");

                        ImGui::SliderFloat("Metallic", &obj.metallic, 0.0f, 1.0f, "%.2f (0=Dielectric, 1=Metal)");

                        ImGui::SliderFloat("Ambient Occlusion", &obj.ambientOcclusion, 0.0f, 1.0f, "%.2f");

                    }

                    

                    ImGui::Spacing();

                    if (ImGui::Button("Load 3D Mesh (.obj, .glb, .gltf, .fbx)", ImVec2(ImGui::GetContentRegionAvail().x, 0)))

                    {

                        const char* filterPatterns[4] = { "*.obj", "*.glb", "*.gltf", "*.fbx" };

                        const char* filePath = tinyfd_openFileDialog("Load 3D Model", "", 4, filterPatterns, "3D Model files", 0);

                        if (filePath)

                        {

                            try {

                                int meshId = assetManager->load3DModelAsset(filePath);

                                if (meshId >= 0)

                                {

                                    obj.meshId = meshId;

                                    obj.modelPath = filePath;

                                    const auto& meshes = assetManager->getMeshes();

                                    if (meshId < static_cast<int>(meshes.size()) && meshes[meshId].defaultTextureId >= 0) {

                                        obj.textureId = meshes[meshId].defaultTextureId;

                                        obj.roughness = meshes[meshId].defaultRoughness;

                                        obj.metallic = meshes[meshId].defaultMetallic;

                                    }

                                    obj.autoAttachAnimatorIfSkinned(meshes);

                                }

                            }

                            catch (const std::exception& e) {

                                std::string msg = e.what();

                                if (msg.rfind("ANIMATION_ONLY_FBX:", 0) == 0)

                                {

                                    obj.autoAttachAnimatorIfSkinned(assetManager->getMeshes());

                                    auto animComp = obj.getAnimator();

                                    auto clip = AnimationLoader::loadFBXClip(filePath, animComp ? animComp->skeleton.get() : nullptr);

                                    if (clip && animComp)

                                    {

                                        animComp->addClip(clip);

                                        animComp->play(clip->name, true);

                                        tinyfd_messageBox("Animation Imported", ("Đã nạp Animation Clip '" + clip->name + "' vào Animator của '" + obj.name + "'!").c_str(), "ok", "info", 1);

                                    }

                                    else

                                    {

                                        tinyfd_messageBox("Animation Clip", ("Tệp '" + std::filesystem::path(filePath).filename().string() + "' là Animation Clip. Hãy dùng 'Import Mixamo FBX Animation' trong Animator Component!").c_str(), "ok", "info", 1);

                                    }

                                }

                                else

                                {

                                    tinyfd_messageBox("Error", e.what(), "ok", "error", 1);

                                }

                            }

                        }

                    }

                    

                    if (ImGui::Button("Load Material Texture", ImVec2(ImGui::GetContentRegionAvail().x, 0)))

                    {

                        const char* filterPatterns[2] = { "*.png", "*.jpg" };

                        const char* filePath = tinyfd_openFileDialog("Load Texture", "", 2, filterPatterns, "Image Files", 0);

                        if (filePath)

                        {

                            try {

                                int texId = assetManager->loadTextureAsset(filePath);

                                if (texId >= 0) {

                                    obj.textureId = texId;

                                    obj.texturePath = filePath;

                                }

                            }

                            catch (const std::exception& e) {

                                tinyfd_messageBox("Error", e.what(), "ok", "error", 1);

                            }

                        }

                    }



                    const auto& meshes = assetManager->getMeshes();

                    const auto& textures = assetManager->getTextures();

                    int effectiveTex = obj.textureId >= 0 ? obj.textureId : (obj.meshId >= 0 && obj.meshId < static_cast<int>(meshes.size()) ? meshes[obj.meshId].defaultTextureId : -1);

                    if (obj.meshId >= 0 && obj.meshId < static_cast<int>(meshes.size()) && !meshes[obj.meshId].submeshes.empty() && obj.textureId < 0) {

                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Multi-Material: %zu Submeshes (Native Textures)", meshes[obj.meshId].submeshes.size());

                    } else if (effectiveTex >= 0 && effectiveTex < static_cast<int>(textures.size())) {

                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Active Texture: Slot %d", effectiveTex);

                        ImGui::SameLine();

                        if (ImGui::SmallButton("Clear")) {

                            obj.textureId = -1;

                        }

                    } else {

                        ImGui::TextDisabled("Active Texture: Default White (None)");

                    }



                    if (obj.name != "Player Cube" && obj.name != "Gold Collectible")

                    {

                        ImGui::Spacing();

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

                        if (ImGui::Button("Remove Mesh Renderer", ImVec2(-1, 24)))

                        {

                            obj.removeComponent(ComponentType::MESH_RENDERER);

                            obj.meshId = -1;

                        }

                        ImGui::PopStyleColor();

                    }

                }

                ImGui::Spacing();

            }



            // 3. RigidBody Physics Component

            if (obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS) || obj.isPhysicsEnabled)

            {

                if (ImGui::CollapsingHeader("RigidBody Physics Component", ImGuiTreeNodeFlags_DefaultOpen))

                {

                    auto rb = obj.getComponent<RigidBodyComponent>();



                    const char* colliderNames[] = { "Box", "Sphere", "Capsule", "Plane" };

                    int colliderIdx = rb ? static_cast<int>(rb->colliderType) : 0;

                    if (ImGui::Combo("Collider Type", &colliderIdx, colliderNames, IM_ARRAYSIZE(colliderNames)))

                    {

                        if (rb) rb->colliderType = static_cast<ColliderType>(colliderIdx);

                    }



                    const char* motionNames[] = { "Static", "Kinematic", "Dynamic" };

                    int motionIdx = rb ? static_cast<int>(rb->motionType) : 2;

                    if (ImGui::Combo("Motion Type", &motionIdx, motionNames, IM_ARRAYSIZE(motionNames)))

                    {

                        if (rb) rb->motionType = static_cast<BodyMotionType>(motionIdx);

                    }



                    ImGui::Separator();



                    float mass = rb ? rb->mass : 1.0f;

                    if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.01f, 1000.0f, "%.2f kg"))

                    {

                        if (rb) rb->mass = mass;

                    }



                    float friction = rb ? rb->friction : 0.5f;

                    if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f, "%.2f"))

                    {

                        if (rb) rb->friction = friction;

                    }



                    float restitution = rb ? rb->restitution : 0.3f;

                    if (ImGui::SliderFloat("Restitution (Bounce)", &restitution, 0.0f, 1.0f, "%.2f"))

                    {

                        if (rb) rb->restitution = restitution;

                    }



                    ImGui::Separator();



                    float linearDrag = rb ? rb->linearDrag : 0.01f;

                    if (ImGui::DragFloat("Linear Drag", &linearDrag, 0.001f, 0.0f, 10.0f, "%.3f"))

                    {

                        if (rb) rb->linearDrag = linearDrag;

                    }



                    float angularDrag = rb ? rb->angularDrag : 0.05f;

                    if (ImGui::DragFloat("Angular Drag", &angularDrag, 0.001f, 0.0f, 10.0f, "%.3f"))

                    {

                        if (rb) rb->angularDrag = angularDrag;

                    }



                    ImGui::Separator();



                    bool useGravity = rb ? rb->useGravity : true;

                    if (ImGui::Checkbox("Use Gravity", &useGravity))

                    {

                        if (rb) rb->useGravity = useGravity;

                    }

                    ImGui::SameLine();

                    ImGui::Checkbox("Enable Physics", &obj.isPhysicsEnabled);



                    bool isTrigger = rb ? rb->isTrigger : false;

                    if (ImGui::Checkbox("Is Trigger", &isTrigger))

                    {

                        if (rb) rb->isTrigger = isTrigger;

                    }



                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f),

                        "Velocity: (%.2f, %.2f, %.2f)", obj.velocity.x, obj.velocity.y, obj.velocity.z);



                    ImGui::Spacing();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

                    if (ImGui::Button("Remove RigidBody Component", ImVec2(-1, 24)))

                    {

                        obj.removeComponent(ComponentType::RIGIDBODY_PHYSICS);

                        obj.isPhysicsEnabled = false;

                    }

                    ImGui::PopStyleColor();

                }

                ImGui::Spacing();

            }



            // 4. Lua Script Components

            if (!obj.luaScripts.empty())

            {

                int scriptToRemove = -1;

                for (int si = 0; si < static_cast<int>(obj.luaScripts.size()); si++)

                {

                    std::string headerLabel = "Lua Script #" + std::to_string(si + 1) + "##lua_" + std::to_string(si);

                    if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))

                    {

                        ImGui::TextDisabled("Edit OOP script or drag a .lua file below.");

                        std::string inputId = "##LuaScript_" + std::to_string(si);

                        static char scriptBuf[8192];

                        strncpy(scriptBuf, obj.luaScripts[si].c_str(), sizeof(scriptBuf));

                        scriptBuf[sizeof(scriptBuf) - 1] = '\0';

                        if (ImGui::InputTextMultiline(inputId.c_str(), scriptBuf, sizeof(scriptBuf), ImVec2(-1.0f, 150.0f), ImGuiInputTextFlags_AllowTabInput))

                        {

                            obj.luaScripts[si] = scriptBuf;

                            if (mode == AppMode::PLAY && scriptEngine) scriptEngine->reloadLuaScripts(scene);

                        }



                        if (ImGui::BeginDragDropTarget())

                        {

                            if (const ImGuiPayload* payloadLua = ImGui::AcceptDragDropPayload("DND_ASSET_LUA"))

                            {

                                const char* assetPath = static_cast<const char*>(payloadLua->Data);

                                std::ifstream t(assetPath);

                                if (t.is_open())

                                {

                                    std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

                                    obj.luaScripts[si] = scriptContent;

                                    if (mode == AppMode::PLAY && scriptEngine) scriptEngine->reloadLuaScripts(scene);

                                }

                            }

                            ImGui::EndDragDropTarget();

                        }



                        ImGui::Spacing();

                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

                        std::string removeLabel = "Remove Script #" + std::to_string(si + 1) + "##rm_lua_" + std::to_string(si);

                        if (ImGui::Button(removeLabel.c_str(), ImVec2(-1, 24)))

                        {

                            scriptToRemove = si;

                        }

                        ImGui::PopStyleColor();

                    }

                    ImGui::Spacing();

                }

                if (scriptToRemove >= 0)

                {

                    obj.luaScripts.erase(obj.luaScripts.begin() + scriptToRemove);

                    if (obj.luaScripts.empty())

                        obj.removeComponent(ComponentType::LUA_SCRIPT);

                }

            }



            // 5. Light Component

            if (obj.hasComponent(ComponentType::LIGHT) || obj.type == ObjectType::LIGHT)

            {

                if (ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen))

                {

                    ImGui::ColorEdit4("Light Color", &obj.color.x);

                    bool shadows = renderer->isShadowMappingEnabled();

                    if (ImGui::Checkbox("Cast Surface Shadows", &shadows))

                    {

                        renderer->setShadowMappingEnabled(shadows);

                    }

                }

                ImGui::Spacing();

            }



            // 6. Animator Component

            auto animComp = obj.getAnimator();

            if (animComp)

            {

                if (ImGui::CollapsingHeader("🎭 Animator Component", ImGuiTreeNodeFlags_DefaultOpen))

                {

                    if (animComp->skeleton)

                    {

                        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Skeleton: %zu Bones", animComp->skeleton->bones.size());

                    }

                    else

                    {

                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "No Skeleton assigned");

                    }



                    // Available Clips Selection

                    if (!animComp->clipNames.empty())

                    {

                        int currentIdx = -1;

                        for (size_t i = 0; i < animComp->clipNames.size(); ++i)

                        {

                            if (animComp->clipNames[i] == animComp->currentClipName)

                            {

                                currentIdx = static_cast<int>(i);

                                break;

                            }

                        }



                        std::string previewName = (currentIdx >= 0) ? animComp->clipNames[currentIdx] : "None";

                        if (ImGui::BeginCombo("Active Clip", previewName.c_str()))

                        {

                            for (size_t i = 0; i < animComp->clipNames.size(); ++i)

                            {

                                bool isSelected = (currentIdx == static_cast<int>(i));

                                if (ImGui::Selectable(animComp->clipNames[i].c_str(), isSelected))

                                {

                                    animComp->crossFade(animComp->clipNames[i], 0.25f, animComp->isLooping);

                                }

                                if (isSelected) ImGui::SetItemDefaultFocus();

                            }

                            ImGui::EndCombo();

                        }



                        // Playback Controls

                        ImGui::Spacing();

                        if (animComp->isPlaying)

                        {

                            if (ImGui::Button("⏸ Pause", ImVec2(75, 24)))

                            {

                                animComp->pause();

                            }

                        }

                        else

                        {

                            if (ImGui::Button("▶ Play", ImVec2(75, 24)))

                            {

                                animComp->resume();

                            }

                        }

                        ImGui::SameLine();

                        if (ImGui::Button("⏹ Stop", ImVec2(75, 24)))

                        {

                            animComp->stop();

                        }

                        ImGui::SameLine();

                        ImGui::Checkbox("Loop", &animComp->isLooping);



                        // Timeline Scrubber

                        auto activeClip = animComp->getClip(animComp->currentClipName);

                        float maxTime = activeClip ? activeClip->duration : 1.0f;

                        float scrubTime = animComp->currentTime;

                        if (ImGui::SliderFloat("Timeline", &scrubTime, 0.0f, maxTime, "%.2f s"))

                        {

                            animComp->seek(scrubTime);

                        }



                        ImGui::SliderFloat("Playback Speed", &animComp->playbackSpeed, 0.1f, 3.0f, "%.2fx");

                    }

                    else

                    {

                        ImGui::TextDisabled("No animation clips loaded.");

                    }



                    ImGui::Spacing();

                    ImGui::Separator();



                    // Shooting Target Controller Controls

                    if (ImGui::TreeNodeEx("🎯 Shooting Target Controller", ImGuiTreeNodeFlags_DefaultOpen))

                    {

                        ImGui::Checkbox("Enable Target Logic", &animComp->isShootingTargetMode);

                        if (animComp->isShootingTargetMode)

                        {

                            const char* stateNames[] = { "READY_UP (Sẵn sàng)", "HIT_FALL (Bị bắn ngã)", "DOWN (Nằm yên)", "RESET_RISE (Dựng dậy)" };

                            int stateIdx = static_cast<int>(animComp->targetState);

                            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.6f, 1.0f), "State: %s", (stateIdx >= 0 && stateIdx <= 3) ? stateNames[stateIdx] : "Unknown");



                            ImGui::SliderFloat("Auto Reset Delay", &animComp->autoResetDelay, 0.5f, 10.0f, "%.1fs");



                            if (ImGui::Button("💥 Trigger Hit (Ngã)", ImVec2(140, 26)))

                            {

                                animComp->onHit();

                            }

                            ImGui::SameLine();

                            if (ImGui::Button("🔄 Reset Target (Dựng)", ImVec2(150, 26)))

                            {

                                animComp->resetTarget();

                            }

                        }

                        ImGui::TreePop();

                    }



                    ImGui::Spacing();

                    ImGui::Separator();



                    // Mixamo FBX Import Button

                    if (ImGui::Button("➕ Import Mixamo FBX Animation...", ImVec2(-1, 28)))

                    {

                        const char* filterPatterns[1] = { "*.fbx" };

                        const char* filePath = tinyfd_openFileDialog("Import Mixamo FBX Clip", "assets/animations/", 1, filterPatterns, "FBX Animation Clip", 0);

                        if (filePath)

                        {

                            try

                            {

                                auto newClip = AnimationLoader::loadFBXClip(filePath, animComp->skeleton.get());

                                if (newClip)

                                {

                                    animComp->addClip(newClip);

                                    animComp->play(newClip->name, true);

                                    const auto& meshes = assetManager->getMeshes();

                                    if (obj.meshId >= 0 && obj.meshId < static_cast<int>(meshes.size()))

                                    {

                                        const_cast<Mesh&>(meshes[obj.meshId]).defaultClips.push_back(newClip);

                                    }

                                }

                                else

                                {

                                    tinyfd_messageBox("Import Animation", "Could not extract animation clip from FBX file.", "ok", "warning", 1);

                                }

                            }

                            catch (const std::exception& e)

                            {

                                tinyfd_messageBox("Animation Load Error", e.what(), "ok", "error", 1);

                            }

                        }

                    }



                    if (ImGui::Button("📂 Import All Animations in Folder...", ImVec2(-1, 26)))

                    {

                        const char* folderPath = tinyfd_selectFolderDialog("Select Animations Folder", "assets/animations/");

                        if (folderPath && std::filesystem::exists(folderPath))

                        {

                            int count = 0;

                            for (const auto& entry : std::filesystem::directory_iterator(folderPath))

                            {

                                if (!entry.is_regular_file()) continue;

                                std::string ext = entry.path().extension().string();

                                for (char& c : ext) c = static_cast<char>(tolower(c));

                                if (ext == ".fbx")

                                {

                                    auto clip = AnimationLoader::loadFBXClip(entry.path().string(), animComp->skeleton.get());

                                    if (clip)

                                    {

                                        animComp->addClip(clip);

                                        count++;

                                    }

                                }

                            }

                            if (count > 0 && !animComp->isPlaying && !animComp->clipNames.empty())

                            {

                                animComp->play(animComp->clipNames.back(), true);

                            }

                            tinyfd_messageBox("Folder Imported", ("Đã nạp thành công " + std::to_string(count) + " animation clips!").c_str(), "ok", "info", 1);

                        }

                    }



                    if (ImGui::Button("📦 Import Animation Pack (.zip)...", ImVec2(-1, 26)))

                    {

                        const char* filterPatterns[1] = { "*.zip" };

                        const char* zipPath = tinyfd_openFileDialog("Chọn Gói Zip Animation", "", 1, filterPatterns, "ZIP Archive (*.zip)", 0);

                        if (zipPath)

                        {

                            importZipAssetPack(zipPath, assetManager, scene);

                        }

                    }



                    ImGui::Spacing();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

                    if (ImGui::Button("Remove Animator Component", ImVec2(-1, 24)))

                    {

                        obj.removeComponent(ComponentType::ANIMATOR);

                    }

                    ImGui::PopStyleColor();

                }

                ImGui::Spacing();

            }



            // Add Component Button

            ImGui::Separator();

            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.55f, 0.35f, 1.0f));

            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.7f, 0.45f, 1.0f));

            if (ImGui::Button(" + Add Component ", ImVec2(-1, 32)))

            {

                ImGui::OpenPopup("AddComponentPopup");

            }

            ImGui::PopStyleColor(2);



            if (ImGui::BeginPopup("AddComponentPopup"))

            {

                ImGui::TextDisabled("-- Add New Component --");

                ImGui::Separator();



                if (!obj.hasComponent(ComponentType::MESH_RENDERER))

                {

                    if (ImGui::MenuItem("Mesh Renderer Component"))

                    {

                        auto meshComp = std::make_shared<MeshRendererComponent>();

                        meshComp->meshId = assetManager->getCubeMeshId();

                        obj.meshId = assetManager->getCubeMeshId();

                        obj.components.push_back(meshComp);

                    }

                }

                if (!obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS))

                {

                    if (ImGui::MenuItem("RigidBody Physics Component"))

                    {

                        auto rbComp = std::make_shared<RigidBodyComponent>();

                        if (obj.type == ObjectType::SPHERE) rbComp->colliderType = ColliderType::SPHERE;

                        else if (obj.type == ObjectType::PLANE) rbComp->colliderType = ColliderType::PLANE;

                        else rbComp->colliderType = ColliderType::BOX;

                        rbComp->motionType = BodyMotionType::DYNAMIC;

                        obj.isPhysicsEnabled = true;

                        obj.components.push_back(rbComp);

                    }

                }

                if (!obj.hasComponent(ComponentType::ANIMATOR))

                {

                    if (ImGui::MenuItem("🎭 Animator Component"))

                    {

                        auto anim = std::make_shared<AnimatorComponent>();

                        const auto& meshes = assetManager->getMeshes();

                        if (obj.meshId >= 0 && obj.meshId < static_cast<int>(meshes.size()))

                        {

                            const auto& m = meshes[obj.meshId];

                            if (m.skeleton) anim->skeleton = m.skeleton;

                            for (const auto& c : m.defaultClips) if (c) anim->addClip(c);

                        }

                        obj.components.push_back(anim);

                    }

                }

                {

                    if (ImGui::MenuItem("Add Lua Script"))

                    {

                        std::string defaultScript = "-- Lua Script #" + std::to_string(obj.luaScripts.size() + 1) + "\n";

                        defaultScript += "local Script = {}\n\n";

                        defaultScript += "function Script:onStart(obj)\n";

                        defaultScript += "    print(\"[Lua] Script started on: \" .. obj.name)\n";

                        defaultScript += "end\n\n";

                        defaultScript += "function Script:onUpdate(obj, dt)\n";

                        defaultScript += "end\n\n";

                        defaultScript += "return Script\n";

                        obj.luaScripts.push_back(defaultScript);

                        if (!obj.hasComponent(ComponentType::LUA_SCRIPT))

                        {

                            auto luaComp = std::make_shared<LuaScriptComponent>();

                            obj.components.push_back(luaComp);

                        }

                    }

                }

                if (!obj.hasComponent(ComponentType::LIGHT))

                {

                    if (ImGui::MenuItem("Light Component"))

                    {

                        auto lightComp = std::make_shared<LightComponent>();

                        obj.type = ObjectType::LIGHT;

                        obj.components.push_back(lightComp);

                    }

                }

                ImGui::EndPopup();

            }



            ImGui::Spacing();



            // Delete Entity button

            if (obj.name != "Player Cube" && obj.name != "Gold Collectible" && obj.name != "Ground Obstacle")

            {

                ImGui::Separator();

                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));

                if (ImGui::Button(" Delete Entity (GameObject) ", ImVec2(-1, 30)))

                {

                    scene->saveHistory();

                    sceneObjects.erase(sceneObjects.begin() + selectedObjectIndex);

                    scene->setSelectedObjectIndex(0);

                }

                ImGui::PopStyleColor(2);

            }

        }

        else if (selectedObjectIndex == -1) // Main Camera

        {

            ImGui::Text("Selected Object: ");

            ImGui::SameLine();

            ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Main Camera");

            ImGui::Separator();



            if (ImGui::Button(" 🎯 Focus Camera Đến Vị Trí Này (Phím F) ", ImVec2(-1, 26)))

            {

                if (gizmoSystem)

                {

                    gizmoSystem->focusOnObject(-1, *scene);

                }

            }



            ImGui::Checkbox("Static", &isStatic);

            ImGui::SameLine();

            ImGui::TextDisabled("|");

            ImGui::SameLine();

            ImGui::Text("Tag:");

            ImGui::SameLine();

            ImGui::SetNextItemWidth(90);

            snprintf(tagBuf, sizeof(tagBuf), "MainCamera");

            ImGui::InputText("##Tag", tagBuf, sizeof(tagBuf));



            ImGui::Spacing();

            ImGui::Separator();

            ImGui::Spacing();



            if (ImGui::CollapsingHeader("Main Camera Transform (Khung 2.4m × 2.0m)", ImGuiTreeNodeFlags_DefaultOpen))

            {

                ImGui::DragFloat3("Position (X,Y,Z)", &gizmoSystem->mainCameraPos.x, 0.05f);

                ImGui::DragFloat3("Target (X,Y,Z)", &gizmoSystem->mainCameraTarget.x, 0.05f);



                if (ImGui::Button("Reset Main Camera"))

                {

                    gizmoSystem->mainCameraPos = glm::vec3(0.0f, 3.2f, 14.5f);

                    gizmoSystem->mainCameraTarget = glm::vec3(0.0f, 1.2f, 0.0f);

                    gizmoSystem->mainCameraFov = 45.0f;

                    gizmoSystem->gameAspectRatioMode = GameViewAspectRatioMode::PHOTO_2M4_X_2M;

                }

            }



            if (ImGui::CollapsingHeader("Editor Scene Camera (Góc Cam Soi Editor)", ImGuiTreeNodeFlags_DefaultOpen))

            {

                ImGui::SliderFloat("Góc Nghiêng Pitch (X)", &gizmoSystem->sceneRotationX, -89.0f, 89.0f, "%.1f°");

                ImGui::SliderFloat("Góc Xoay Yaw (Y)", &gizmoSystem->sceneRotationY, 0.0f, 360.0f, "%.1f°");

                ImGui::DragFloat("Khoảng Cách (Zoom)", &gizmoSystem->sceneCameraDistance, 0.1f, 1.0f, 60.0f, "%.1f m");

                ImGui::DragFloat3("Tâm Xoay Pivot Target", &gizmoSystem->sceneCameraTarget.x, 0.05f);

            }



            if (ImGui::CollapsingHeader("Camera Lens Settings", ImGuiTreeNodeFlags_DefaultOpen))

            {

                ImGui::SliderFloat("Field of View (FOV)", &gizmoSystem->mainCameraFov, 10.0f, 120.0f);

                ImGui::DragFloat("Near Clip", &gizmoSystem->mainCameraNear, 0.01f, 0.01f, 5.0f);

                ImGui::DragFloat("Far Clip", &gizmoSystem->mainCameraFar, 0.5f, 5.0f, 200.0f);

            }



            if (ImGui::CollapsingHeader("🖼️ Khung Hình Chiếu 2.4m × 2.0m (View Ảnh)", ImGuiTreeNodeFlags_DefaultOpen))

            {

                const char* ratioNames[] = {

                    "🖼️ 2.4m × 2.0m (Chuẩn View Ảnh / Màn Chiếu 6:5)",

                    "📺 16:9 (Chuẩn Màn Hình)",

                    "📐 Tự Do (Fill Cửa Sổ)"

                };

                int currentRatio = static_cast<int>(gizmoSystem->gameAspectRatioMode);

                if (ImGui::Combo("Tỉ Lệ Khung", &currentRatio, ratioNames, IM_ARRAYSIZE(ratioNames)))

                {

                    gizmoSystem->gameAspectRatioMode = static_cast<GameViewAspectRatioMode>(currentRatio);

                }



                ImGui::Spacing();

                if (ImGui::Button("📷 Căn Cam Chiếu Trực Diện Sinh Cảnh (2.4m × 2.0m)", ImVec2(-1, 30)))

                {

                    gizmoSystem->frameScenicPhotoView(*scene, 0);

                }



                ImGui::Text("Góc máy mẫu:");

                if (ImGui::Button("Chính Diện", ImVec2(75, 24))) gizmoSystem->frameScenicPhotoView(*scene, 0);

                ImGui::SameLine();

                if (ImGui::Button("Nghiêng 45°", ImVec2(80, 24))) gizmoSystem->frameScenicPhotoView(*scene, 1);

                ImGui::SameLine();

                if (ImGui::Button("Tầm Mắt", ImVec2(65, 24))) gizmoSystem->frameScenicPhotoView(*scene, 2);

                ImGui::SameLine();

                if (ImGui::Button("Góc Cao", ImVec2(65, 24))) gizmoSystem->frameScenicPhotoView(*scene, 3);



                ImGui::Spacing();

                ImGui::Checkbox("Hiển Thị Thước Đo 2.4m × 2.0m", &gizmoSystem->showPhotoFrameRulers);

                ImGui::Checkbox("Hiển Thị Lưới Bố Cục 1/3 (Rule of Thirds)", &gizmoSystem->showPhotoGrid);



                ImGui::Spacing();

                ImGui::Separator();

                ImGui::Spacing();



                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.65f, 0.32f, 0.95f));

                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.80f, 0.40f, 1.0f));

                if (ImGui::Button("💾 Xuất Ảnh Khung Cam (PNG / JPG)", ImVec2(-1, 32)))

                {

                    if (renderer)

                    {

                        bool is2m4 = (gizmoSystem->gameAspectRatioMode == GameViewAspectRatioMode::PHOTO_2M4_X_2M);

                        uint32_t expW = is2m4 ? 2400 : 1920;

                        uint32_t expH = is2m4 ? 2000 : 1080;

                        const char* filterPatterns[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };

                        std::string defaultName = is2m4 ? "Cam_View_2m4x2m.png" : "Cam_View.png";

                        const char* savePath = tinyfd_saveFileDialog(

                            "Lưu Ảnh Khung Cam (Export Camera View Image)",

                            defaultName.c_str(),

                            4,

                            filterPatterns,

                            "Image Files (*.png, *.jpg, *.bmp)"

                        );

                        if (savePath)

                        {

                            if (renderer->exportGameViewImage(savePath, expW, expH))

                            {

                                gizmoSystem->lastExportedImagePath = savePath;

                                gizmoSystem->exportNotificationTimer = 7.0f;

                            }

                        }

                    }

                }

                ImGui::PopStyleColor(2);

            }

        }



        ImGui::PopStyleVar();

    }

    ImGui::End();

}



void EditorUI::drawConsolePanel(const std::string& selectedGpuName, AppMode mode, const Scene* scene,

                                float editorHeight, float windowWidth)

{

    ImGui::SetNextWindowPos(ImVec2(0.0f, editorHeight - bottomPanelHeight), ImGuiCond_Always);

    ImGui::SetNextWindowSize(ImVec2(windowWidth, bottomPanelHeight), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Console / Project Logs", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))

    {

        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.4f, 1.0f), "[INFO] Render Device: %s", selectedGpuName.c_str());

        if (mode == AppMode::PLAY) {

            ImGui::TextColored(ImVec4(0.1f, 0.9f, 0.3f, 1.0f), "[GAMEPLAY] Playing! WASD or Arrow Keys to move the Player Cube. Space to jump. Collect the gold target!");

        } else {

            ImGui::TextDisabled("[CAMERA] Adjust Main Camera Pos (X,Y,Z) in Inspector or select Main Camera in Hierarchy.");

            ImGui::Text("[SCENE] Drag inside Scene window to rotate view. Scroll to zoom. Add/remove/edit objects in Hierarchy/Inspector.");

        }

        ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "[SYSTEM] App mode: %s. Objects count: %d", mode == AppMode::PLAY ? "PLAY" : "EDIT", scene ? (int)scene->getObjects().size() : 0);

    }

    ImGui::End();

}



void EditorUI::drawLoadSceneModal(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem,

                                  VulkanRenderer* renderer, PhysEngine* physEngine, ScriptEngine* scriptEngine, AppMode& mode)

{

    struct SceneFileInfo {

        std::string fileName;

        std::string filePath;

        std::string envName;

        size_t objectCount = 0;

        uintmax_t fileSize = 0;

        std::string icon = "📁 ";

    };

    static std::vector<SceneFileInfo> cachedSceneFiles;



    auto refreshSceneFiles = [&]() {

        cachedSceneFiles.clear();

        std::vector<std::string> searchDirs = { ".", "scenes", "build/Release" };

        std::unordered_map<std::string, bool> seenStems;



        for (const auto& dir : searchDirs)

        {

            if (!std::filesystem::exists(dir)) continue;

            for (const auto& entry : std::filesystem::directory_iterator(dir))

            {

                if (!entry.is_regular_file()) continue;

                std::string ext = entry.path().extension().string();

                for (auto& c : ext) c = static_cast<char>(tolower(c));

                if (ext != ".json") continue;



                std::string fname = entry.path().filename().string();

                std::string lowerFname = fname;

                for (auto& c : lowerFname) c = static_cast<char>(tolower(c));



                if (lowerFname.find("test_") != std::string::npos || lowerFname.find("compile_commands") != std::string::npos || lowerFname.find("cmake") != std::string::npos) continue;



                std::string stem = entry.path().stem().string();

                if (seenStems[stem]) continue;

                seenStems[stem] = true;



                std::string normPath = entry.path().string();

                std::replace(normPath.begin(), normPath.end(), '\\', '/');



                SceneFileInfo info;

                info.fileName = fname;

                info.filePath = normPath;

                info.fileSize = entry.file_size();



                std::ifstream f(normPath);

                if (f.is_open())

                {

                    std::string line;

                    bool isShapeRendererScene = false;

                    while (std::getline(f, line))

                    {

                        if (line.find("# ShapeRenderer Scene File") != std::string::npos) isShapeRendererScene = true;

                        if (line.rfind("environment: ", 0) == 0)

                        {

                            info.envName = line.substr(13);

                        }

                        if (line.rfind("object: ", 0) == 0)

                        {

                            info.objectCount++;

                        }

                    }

                    if (!isShapeRendererScene && info.objectCount == 0 && fname != "scene.json")

                    {

                        continue;

                    }

                }



                if (info.envName.empty())

                {

                    if (lowerFname.find("warehouse") != std::string::npos) info.envName = "Industrial Warehouse";

                    else if (lowerFname.find("nyc") != std::string::npos || lowerFname.find("street") != std::string::npos || lowerFname.find("urban") != std::string::npos) info.envName = "NYC Street Scene";

                    else if (lowerFname.find("forest") != std::string::npos) info.envName = "Forest Clearing";

                    else if (lowerFname.find("tactical") != std::string::npos || lowerFname.find("outpost") != std::string::npos) info.envName = "Tactical Outpost";

                    else if (lowerFname.find("desert") != std::string::npos || lowerFname.find("dune") != std::string::npos) info.envName = "Desert Dunes";

                    else if (lowerFname.find("city") != std::string::npos || lowerFname.find("cyber") != std::string::npos) info.envName = "Cyberpunk City";

                    else if (lowerFname.find("medieval") != std::string::npos || lowerFname.find("citadel") != std::string::npos) info.envName = "Medieval Citadel";

                    else if (lowerFname.find("island") != std::string::npos) info.envName = "Floating Islands";

                    else if (lowerFname.find("maze") != std::string::npos) info.envName = "3D Maze";

                    else if (fname == "scene.json") info.envName = "Default Scene (scene.json)";

                    else info.envName = stem;

                }



                std::string lowerEnv = info.envName;

                for (auto& c : lowerEnv) c = static_cast<char>(tolower(c));

                if (lowerEnv.find("warehouse") != std::string::npos) info.icon = "🏭 ";

                else if (lowerEnv.find("nyc") != std::string::npos || lowerEnv.find("street") != std::string::npos || lowerEnv.find("urban") != std::string::npos) info.icon = "🏙️ ";

                else if (lowerEnv.find("forest") != std::string::npos) info.icon = "🌲 ";

                else if (lowerEnv.find("tactical") != std::string::npos || lowerEnv.find("outpost") != std::string::npos || lowerEnv.find("target") != std::string::npos) info.icon = "🎯 ";

                else if (lowerEnv.find("desert") != std::string::npos || lowerEnv.find("dune") != std::string::npos) info.icon = "🏜️ ";

                else if (lowerEnv.find("city") != std::string::npos || lowerEnv.find("cyber") != std::string::npos) info.icon = "🏙️ ";

                else if (lowerEnv.find("medieval") != std::string::npos || lowerEnv.find("citadel") != std::string::npos) info.icon = "🏰 ";

                else if (lowerEnv.find("island") != std::string::npos) info.icon = "🌌 ";

                else if (lowerEnv.find("maze") != std::string::npos) info.icon = "🌀 ";

                else info.icon = "📁 ";



                cachedSceneFiles.push_back(info);

            }

        }



        if (loadSceneSelectedPath.empty() && !cachedSceneFiles.empty())

        {

            loadSceneSelectedPath = cachedSceneFiles[0].filePath;

        }

    };



    if (showLoadSceneModal)

    {

        ImGui::OpenPopup("📂 Chọn Scene Để Tải (Select Scene to Load)");

        showLoadSceneModal = false;

        refreshSceneFiles();

    }



    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowSize(ImVec2(700, 480), ImGuiCond_Appearing);



    bool isLoadOpen = true;

    if (ImGui::BeginPopupModal("📂 Chọn Scene Để Tải (Select Scene to Load)", &isLoadOpen, ImGuiWindowFlags_NoCollapse))

    {

        if (!isLoadOpen)

        {

            ImGui::CloseCurrentPopup();

        }



        ImGui::TextColored(ImVec4(0.96f, 0.65f, 0.14f, 1.0f), "✨ Danh Sách Scene Hiện Có Trong Dự Án (Available Scenes):");

        ImGui::TextDisabled("Nhấp đúp chuột vào một scene để nạp ngay, hoặc chọn và bấm 'Tải Scene Đang Chọn':");

        ImGui::SameLine(ImGui::GetWindowWidth() - 170.0f);

        if (ImGui::Button(" 🔄 Làm Mới Danh Sách "))

        {

            refreshSceneFiles();

        }

        ImGui::Separator();



        ImGui::BeginChild("SceneFileList", ImVec2(0, 250), true);

        if (ImGui::BeginTable("SceneTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))

        {

            ImGui::TableSetupColumn("Môi Trường / Tên File", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableSetupColumn("Số Vật Thể", ImGuiTableColumnFlags_WidthFixed, 100.0f);

            ImGui::TableSetupColumn("Dung Lượng", ImGuiTableColumnFlags_WidthFixed, 90.0f);

            ImGui::TableHeadersRow();



            for (const auto& sf : cachedSceneFiles)

            {

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);



                bool isSel = (loadSceneSelectedPath == sf.filePath);

                std::string rowLabel = sf.icon + sf.envName + " (" + sf.fileName + ")";



                if (ImGui::Selectable(rowLabel.c_str(), isSel, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick))

                {

                    loadSceneSelectedPath = sf.filePath;

                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))

                    {

                        scene->loadScene(sf.filePath, assetManager, assetManager->getCubeMeshId(), assetManager->getSphereMeshId(), assetManager->getPlaneMeshId(), gizmoSystem, renderer, physEngine);

                        if (mode == AppMode::PLAY)

                        {

                            scene->initPhysicsBodies(physEngine);

                            scriptEngine->reloadLuaScripts(scene);

                        }

                        ImGui::CloseCurrentPopup();

                    }

                }



                ImGui::TableSetColumnIndex(1);

                ImGui::Text("%zu objects", sf.objectCount);



                ImGui::TableSetColumnIndex(2);

                float kb = sf.fileSize / 1024.0f;

                ImGui::Text("%.1f KB", kb);

            }

            ImGui::EndTable();

        }

        ImGui::EndChild();



        ImGui::Spacing();

        ImGui::Text("Đường dẫn file đã chọn: %s", loadSceneSelectedPath.c_str());

        ImGui::Separator();

        ImGui::Spacing();



        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.35f, 1.0f));

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));

        if (ImGui::Button(" 📂 TẢI SCENE ĐANG CHỌN (LOAD SCENE) ", ImVec2(240, 32)))

        {

            if (!loadSceneSelectedPath.empty())

            {

                scene->loadScene(loadSceneSelectedPath, assetManager, assetManager->getCubeMeshId(), assetManager->getSphereMeshId(), assetManager->getPlaneMeshId(), gizmoSystem, renderer, physEngine);

                if (mode == AppMode::PLAY)

                {

                    scene->initPhysicsBodies(physEngine);

                    scriptEngine->reloadLuaScripts(scene);

                }

            }

            ImGui::CloseCurrentPopup();

        }

        ImGui::PopStyleColor(2);



        ImGui::SameLine();

        if (ImGui::Button(" 🗂️ Chọn File Khác từ Máy Tính... (Browse File) ", ImVec2(270, 32)))

        {

            const char* filterPatterns[] = { "*.json" };

            const char* openPath = tinyfd_openFileDialog(

                "Chọn File Scene JSON",

                "scene.json",

                1,

                filterPatterns,

                "Scene Files (*.json)",

                0

            );

            if (openPath)

            {

                scene->loadScene(openPath, assetManager, assetManager->getCubeMeshId(), assetManager->getSphereMeshId(), assetManager->getPlaneMeshId(), gizmoSystem, renderer, physEngine);

                if (mode == AppMode::PLAY)

                {

                    scene->initPhysicsBodies(physEngine);

                    scriptEngine->reloadLuaScripts(scene);

                }

                ImGui::CloseCurrentPopup();

            }

        }



        ImGui::SameLine();

        if (ImGui::Button(" ✖ Hủy (Cancel) ", ImVec2(100, 32)))

        {

            ImGui::CloseCurrentPopup();

        }



        ImGui::EndPopup();

    }

}



void EditorUI::drawSaveSceneModal(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem, VulkanRenderer* renderer)

{

    if (showSaveSceneModal)

    {

        ImGui::OpenPopup("💾 Lưu Scene Theo Môi Trường (Save Scene As)");

        std::string suggested = Scene::getEnvironmentFileName(scene->getCurrentEnvironmentName());

        strncpy(saveSceneFileNameInput, suggested.c_str(), sizeof(saveSceneFileNameInput) - 1);

        saveSceneFileNameInput[sizeof(saveSceneFileNameInput) - 1] = '\0';

        showSaveSceneModal = false;

    }



    ImVec2 center = ImGui::GetMainViewport()->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    ImGui::SetNextWindowSize(ImVec2(560, 240), ImGuiCond_Appearing);



    bool isSaveOpen = true;

    if (ImGui::BeginPopupModal("💾 Lưu Scene Theo Môi Trường (Save Scene As)", &isSaveOpen, ImGuiWindowFlags_NoCollapse))

    {

        if (!isSaveOpen)

        {

            ImGui::CloseCurrentPopup();

        }



        ImGui::TextColored(ImVec4(0.96f, 0.65f, 0.14f, 1.0f), "💾 Lưu File Scene Theo Môi Trường Đang Tạo:");

        ImGui::TextDisabled("Tên file được gợi ý tự động theo môi trường hiện tại:");

        ImGui::Separator();

        ImGui::Spacing();



        ImGui::Text("Môi trường hiện tại: %s", scene->getCurrentEnvironmentName().c_str());

        ImGui::Spacing();



        ImGui::InputText("Tên File Lưu (.json)", saveSceneFileNameInput, sizeof(saveSceneFileNameInput));

        ImGui::Spacing();

        ImGui::Separator();

        ImGui::Spacing();



        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.65f, 0.35f, 1.0f));

        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.4f, 1.0f));

        if (ImGui::Button(" 💾 LƯU SCENE (SAVE) ", ImVec2(180, 32)))

        {

            std::string saveFile = saveSceneFileNameInput;

            if (saveFile.empty()) saveFile = "scene.json";

            if (saveFile.find(".json") == std::string::npos) saveFile += ".json";



            scene->setCurrentScenePath(saveFile);

            scene->saveScene(saveFile, assetManager, gizmoSystem, renderer);

            ImGui::CloseCurrentPopup();

        }

        ImGui::PopStyleColor(2);



        ImGui::SameLine();

        if (ImGui::Button(" 🗂️ Duyệt Nơi Lưu... (Browse File) ", ImVec2(210, 32)))

        {

            const char* filterPatterns[] = { "*.json" };

            const char* savePath = tinyfd_saveFileDialog(

                "Lưu File Scene JSON",

                saveSceneFileNameInput,

                1,

                filterPatterns,

                "Scene Files (*.json)"

            );

            if (savePath)

            {

                scene->setCurrentScenePath(savePath);

                scene->saveScene(savePath, assetManager, gizmoSystem, renderer);

                ImGui::CloseCurrentPopup();

            }

        }



        ImGui::SameLine();

        if (ImGui::Button(" ✖ Hủy (Cancel) ", ImVec2(100, 32)))

        {

            ImGui::CloseCurrentPopup();

        }



        ImGui::EndPopup();

    }

}



