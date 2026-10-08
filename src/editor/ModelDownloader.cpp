#include "editor/ModelDownloader.h"
#include "scene/Scene.h"
#include "assets/AssetManager.h"
#include "imgui.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <thread>
#endif

#include <filesystem>
#include <vector>

ModelDownloader::ModelDownloader()
    : showOnlineDownloaderPanel(true),
      isDownloadingModel(false)
{
}

bool ModelDownloader::downloadModelFromUrl(const std::string& urlStr, const std::string& customFileName, Scene* scene, AssetManager* assetManager)
{
    if (urlStr.empty()) return false;

    std::filesystem::create_directories("assets/models");

    std::string fileName = customFileName;
    if (fileName.empty())
    {
        size_t lastSlash = urlStr.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            fileName = urlStr.substr(lastSlash + 1);
        }
        else
        {
            fileName = "DownloadedModel.glb";
        }
    }

    std::filesystem::path p(fileName);
    std::string ext = p.extension().string();
    if (ext.empty())
    {
        fileName += ".glb";
    }

    std::string destPath = "assets/models/" + fileName;
    downloaderStatusMsg = "⏳ Đang tải từ API URL: " + fileName + "...";
    printf("[Downloader] Requesting API URL: %s -> %s\n", urlStr.c_str(), destPath.c_str());

    isDownloadingModel = true;

#ifdef _WIN32
    HRESULT hr = URLDownloadToFileA(NULL, urlStr.c_str(), destPath.c_str(), 0, NULL);
#else
    HRESULT hr = E_FAIL;
#endif
    isDownloadingModel = false;

#ifdef _WIN32
    if (SUCCEEDED(hr))
    {
        downloaderStatusMsg = "✅ Tải thành công! Đã lưu vào " + destPath;
        printf("[Downloader SUCCESS] Saved model to %s\n", destPath.c_str());

        if (assetManager && scene)
        {
            int mId = assetManager->getOrLoadModelAsset(destPath);
            if (mId >= 0)
            {
                scene->saveHistory();
                SceneObject newObj;
                newObj.id = static_cast<int>(scene->getObjects().size());
                newObj.name = p.stem().string();
                newObj.type = ObjectType::CUBE;
                newObj.position = glm::vec3(0.0f, 1.0f, 0.0f);
                newObj.scale = glm::vec3(1.0f);
                newObj.color = glm::vec4(1.0f);
                newObj.meshId = mId;
                scene->getObjects().push_back(newObj);
                scene->setSelectedObjectIndex(static_cast<int>(scene->getObjects().size()) - 1);
            }
        }
        return true;
    }
    else
    {
        downloaderStatusMsg = "❌ Thất bại khi tải file từ API URL: " + urlStr;
        printf("[Downloader ERROR] Failed HRESULT: 0x%08X\n", (unsigned int)hr);
        return false;
    }
#else
    return false;
#endif
}

void ModelDownloader::draw(Scene* scene, AssetManager* assetManager)
{
    if (!showOnlineDownloaderPanel) return;

    ImGui::SetNextWindowSize(ImVec2(430, 540), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("📥 Tải Model 3D Trực Tuyến (Online Asset Downloader)", &showOnlineDownloaderPanel))
    {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "🌐 Plugin Tải Model 3D Tự Động Từ Link API / Internet");
        ImGui::Separator();

        if (ImGui::BeginTabBar("DownloaderTabs"))
        {
            if (ImGui::BeginTabItem("🌐 Tải từ Link URL Direct"))
            {
                ImGui::Spacing();
                ImGui::Text("Dán URL trực tiếp tới file 3D (.glb, .gltf, .obj):");
                ImGui::InputText("URL Link", downloaderUrlBuffer, IM_ARRAYSIZE(downloaderUrlBuffer));
                ImGui::InputText("Tên file lưu (tùy chọn)", downloaderFilenameBuffer, IM_ARRAYSIZE(downloaderFilenameBuffer));

                ImGui::Spacing();
                if (isDownloadingModel)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "⏳ Đang tải dữ liệu 3D từ internet...");
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.6f, 0.3f, 1.0f));
                    if (ImGui::Button(" 📥 TẢI MODEL VỀ ASSETS & THẢ VÀO 3D SCENE ", ImVec2(-1, 36)))
                    {
                        std::string url = downloaderUrlBuffer;
                        std::string customName = downloaderFilenameBuffer;
                        if (!url.empty())
                        {
                            std::thread([this, url, customName, scene, assetManager]() {
                                downloadModelFromUrl(url, customName, scene, assetManager);
                            }).detach();
                        }
                    }
                    ImGui::PopStyleColor();
                }

                ImGui::Spacing();
                ImGui::TextWrapped("💡 Mẹo: Bạn có thể copy link raw .glb/.obj từ GitHub, Khronos Sample Repository, hoặc link direct từ Sketchfab/PolyPizza.");
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("📚 Kho Catalog Model 3D Miễn Phí (1-Click)"))
            {
                ImGui::Spacing();
                ImGui::TextDisabled("Bấm nút 📥 1-Click Download để tải model thẳng về assets/models/ và hiện lên Scene:");

                std::vector<OnlineCatalogItem> catalog = {
                    { "🌲 Cây Thông (Pine Tree)", "Rừng & Tự Nhiên", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Tree/glTF-Binary/Tree.glb", "PineTree.glb", "Cây thông 3D lá kim xanh rậm" },
                    { "🦊 Con Cáo (Fox 3D)", "Động Vật", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Fox/glTF-Binary/Fox.glb", "Fox.glb", "Mẫu con cáo 3D đáng yêu" },
                    { "🦆 Con Vịt (Duck 3D)", "Động Vật", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Duck/glTF-Binary/Duck.glb", "Duck.glb", "Mẫu con vịt vàng 3D" },
                    { "🧍 Nhân Vật Người (Cesium Man)", "Nhân Vật", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/CesiumMan/glTF-Binary/CesiumMan.glb", "CesiumMan.glb", "Mẫu nhân vật 3D đi bộ" },
                    { "🏮 Đèn Lồng Cổ (Lantern)", "Đồ Vật & Cổ Vật", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Lantern/glTF-Binary/Lantern.glb", "Lantern.glb", "Đèn lồng cổ 3D cao cấp" },
                    { "👑 Mũ Chiến Binh (Damaged Helmet)", "Cổ Vật & Vũ Khí", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/DamagedHelmet/glTF-Binary/DamagedHelmet.glb", "DamagedHelmet.glb", "Mũ bảo hiểm PBR cực đẹp" },
                    { "🥑 Quả Bơ (Avocado)", "Vật Thể", "https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/main/2.0/Avocado/glTF-Binary/Avocado.glb", "Avocado.glb", "Mẫu quả bơ 3D PBR" }
                };

                for (size_t i = 0; i < catalog.size(); ++i)
                {
                    const auto& item = catalog[i];
                    ImGui::PushID(static_cast<int>(i));

                    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", item.name.c_str());
                    ImGui::SameLine();
                    ImGui::TextDisabled("[%s]", item.category.c_str());

                    ImGui::Text("    %s (%s)", item.description.c_str(), item.filename.c_str());

                    if (ImGui::Button(" 📥 1-Click Download & Import "))
                    {
                        std::string u = item.url;
                        std::string f = item.filename;
                        std::thread([this, u, f, scene, assetManager]() {
                            downloadModelFromUrl(u, f, scene, assetManager);
                        }).detach();
                    }
                    ImGui::Separator();
                    ImGui::PopID();
                }

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Trạng thái: %s", downloaderStatusMsg.c_str());
    }
    ImGui::End();
}
