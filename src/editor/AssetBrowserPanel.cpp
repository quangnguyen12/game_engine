#include "editor/AssetBrowserPanel.h"
#include "assets/AssetManager.h"
#include "scene/Scene.h"
#include "imgui.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

#include <filesystem>
#include <fstream>
#include <algorithm>

AssetBrowserPanel::AssetBrowserPanel()
    : showAssetBrowserPanel(true)
{
}

void AssetBrowserPanel::openFileInExternalEditor(const std::string& filePath)
{
#ifdef _WIN32
    ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    std::string cmd = "xdg-open \"" + filePath + "\" &";
    system(cmd.c_str());
#endif
}

void AssetBrowserPanel::draw(AssetManager* assetManager, Scene* scene, float windowWidth, float bottomPanelHeight)
{
    if (!showAssetBrowserPanel || !assetManager || !scene) return;

    ImGui::SetNextWindowPos(ImVec2(0.0f, ImGui::GetIO().DisplaySize.y - bottomPanelHeight), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth, bottomPanelHeight), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("📁 Project Assets & Drag-and-Drop Browser", &showAssetBrowserPanel, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove))
    {
        if (ImGui::BeginTabBar("AssetBrowserTabs"))
        {
            if (ImGui::BeginTabItem("📦 Workspace Assets (assets/)"))
            {
                std::string assetsDir = "assets";
                if (!std::filesystem::exists(assetsDir))
                {
                    std::filesystem::create_directory(assetsDir);
                }

                float cardWidth = 110.0f;
                float cardHeight = 92.0f;
                float availWidth = ImGui::GetContentRegionAvail().x;
                int cols = static_cast<int>(availWidth / (cardWidth + 12.0f));
                if (cols < 1) cols = 1;

                if (ImGui::BeginTable("AssetsGrid", cols, ImGuiTableFlags_SizingFixedFit))
                {
                    int col = 0;
                    auto& sceneObjects = scene->getObjects();
                    int selectedObjectIndex = scene->getSelectedObjectIndex();
                    auto& meshes = assetManager->getMeshes();
                    auto& assetThumbnails = assetManager->getThumbnails();

                    for (const auto& entry : std::filesystem::directory_iterator(assetsDir))
                    {
                        if (entry.is_regular_file())
                        {
                            std::string pathStr = entry.path().string();
                            std::string filenameStr = entry.path().filename().string();
                            std::string ext = entry.path().extension().string();
                            for (auto& c : ext) c = static_cast<char>(tolower(c));

                            if (col == 0) ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(col);

                            ImGui::PushID(pathStr.c_str());

                            ImGui::BeginGroup();
                            ImVec2 p = ImGui::GetCursorScreenPos();
                            ImDrawList* drawList = ImGui::GetWindowDrawList();

                            bool isHovered = ImGui::IsMouseHoveringRect(p, ImVec2(p.x + cardWidth, p.y + cardHeight));
                            ImU32 bgCol = isHovered ? IM_COL32(45, 50, 65, 255) : IM_COL32(30, 32, 38, 255);
                            ImU32 borderCol = isHovered ? IM_COL32(0, 180, 255, 255) : IM_COL32(60, 65, 75, 255);
                            drawList->AddRectFilled(p, ImVec2(p.x + cardWidth, p.y + cardHeight), bgCol, 6.0f);
                            drawList->AddRect(p, ImVec2(p.x + cardWidth, p.y + cardHeight), borderCol, 6.0f, 0, isHovered ? 1.8f : 1.0f);

                            ImGui::Dummy(ImVec2(cardWidth, cardHeight));

                            ImGui::SetCursorScreenPos(p);
                            ImGui::Selectable("##CardSelect", false, 0, ImVec2(cardWidth, cardHeight));

                            if (ImGui::BeginPopupContextItem())
                            {
                                if (ImGui::MenuItem("📖 Edit in External IDE (VS Code / Studio)"))
                                {
                                    openFileInExternalEditor(pathStr);
                                }
                                if (ext == ".lua" && ImGui::MenuItem("➕ Attach Script to Selected Object"))
                                {
                                    if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                                    {
                                        std::ifstream t(pathStr);
                                        if (t.is_open())
                                        {
                                            std::string scriptContent((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
                                            sceneObjects[selectedObjectIndex].luaScripts.push_back(scriptContent);
                                        }
                                    }
                                }
                                ImGui::EndPopup();
                            }

                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                            {
                                if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_MODEL", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("📦 Dragging 3D Model '%s'\nDrop into Scene View to place object!", filenameStr.c_str());
                                }
                                else if (ext == ".lua")
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_LUA", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("📖 Dragging Lua Script '%s'\nDrop into Inspector / Hierarchy to attach script!", filenameStr.c_str());
                                }
                                else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_TEXTURE", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("🖼️ Dragging Texture '%s'\nDrop onto Object in Scene / Inspector to apply!", filenameStr.c_str());
                                }
                                else
                                {
                                    ImGui::SetDragDropPayload("DND_ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
                                    ImGui::Text("📄 Dragging File '%s'", filenameStr.c_str());
                                }
                                ImGui::EndDragDropSource();
                            }

                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            {
                                if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
                                {
                                    scene->saveHistory();
                                    int meshId = assetManager->load3DModelAsset(pathStr);
                                    if (meshId >= 0)
                                    {
                                        SceneObject newObj;
                                        newObj.id = static_cast<int>(sceneObjects.size());
                                        std::string sName = entry.path().stem().string();
                                        newObj.name = sName;
                                        newObj.type = ObjectType::CUBE;
                                        newObj.position = glm::vec3(0.0f);
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
                                        if (meshId < static_cast<int>(meshes.size())) {
                                            newObj.textureId = meshes[meshId].defaultTextureId;
                                            newObj.roughness = meshes[meshId].defaultRoughness;
                                            newObj.metallic = meshes[meshId].defaultMetallic;
                                        }
                                        sceneObjects.push_back(newObj);
                                        scene->setSelectedObjectIndex(static_cast<int>(sceneObjects.size()) - 1);
                                    }
                                }
                                else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                                {
                                    scene->saveHistory();
                                    int texId = assetManager->loadTextureAsset(pathStr);
                                    if (texId >= 0 && selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
                                    {
                                        sceneObjects[selectedObjectIndex].textureId = texId;
                                    }
                                }
                                else if (ext == ".lua")
                                {
                                    openFileInExternalEditor(pathStr);
                                }
                            }

                            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                            {
                                if (assetThumbnails.find(pathStr) == assetThumbnails.end())
                                {
                                    try {
                                        Texture tex;
                                        assetManager->loadTexture(pathStr, tex);
                                        assetThumbnails[pathStr] = tex;
                                    } catch (...) {}
                                }

                                if (assetThumbnails.find(pathStr) != assetThumbnails.end())
                                {
                                    ImGui::SetCursorScreenPos(ImVec2(p.x + (cardWidth - 44.0f) * 0.5f, p.y + 8.0f));
                                    ImGui::Image((ImTextureID)assetThumbnails[pathStr].descriptorSet, ImVec2(44.0f, 44.0f));
                                }
                                else
                                {
                                    ImGui::SetCursorScreenPos(ImVec2(p.x + 28.0f, p.y + 12.0f));
                                    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "🖼️ TEX");
                                }
                            }
                            else if (ext == ".lua")
                            {
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 42.0f, p.y + 10.0f));
                                ImGui::SetWindowFontScale(1.3f);
                                ImGui::Text("📖");
                                ImGui::SetWindowFontScale(1.0f);
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 18.0f, p.y + 36.0f));
                                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "LUA SCRIPT");
                            }
                            else if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
                            {
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 42.0f, p.y + 10.0f));
                                ImGui::SetWindowFontScale(1.3f);
                                ImGui::Text("📦");
                                ImGui::SetWindowFontScale(1.0f);
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 22.0f, p.y + 36.0f));
                                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "3D MODEL");
                            }
                            else
                            {
                                ImGui::SetCursorScreenPos(ImVec2(p.x + 42.0f, p.y + 14.0f));
                                ImGui::Text("📄");
                            }

                            std::string displayFilename = filenameStr;
                            if (displayFilename.size() > 11) displayFilename = displayFilename.substr(0, 9) + "..";
                            ImGui::SetCursorScreenPos(ImVec2(p.x + 6.0f, p.y + cardHeight - 20.0f));
                            ImGui::TextDisabled("%s", displayFilename.c_str());
                            ImGui::EndGroup();
                            ImGui::PopID();
                            col = (col + 1) % cols;
                        }
                    }
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("🎲 Built-in Primitives"))
            {
                struct PrimitiveItem {
                    const char* name;
                    const char* payloadKey;
                    const char* icon;
                };

                PrimitiveItem primitives[] = {
                    { "Cube Primitive", "PRIMITIVE_CUBE", "🎲" },
                    { "Sphere Primitive", "PRIMITIVE_SPHERE", "🔮" },
                    { "Plane Primitive", "PRIMITIVE_PLANE", "📜" }
                };

                for (const auto& item : primitives)
                {
                    std::string label = std::string(item.icon) + " " + item.name;
                    ImGui::Selectable(label.c_str(), false, 0, ImVec2(180, 26));

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        ImGui::SetDragDropPayload("DND_ASSET_MODEL", item.payloadKey, strlen(item.payloadKey) + 1);
                        ImGui::Text("🎲 Dragging '%s'\nDrop into Scene View to place!", item.name);
                        ImGui::EndDragDropSource();
                    }

                    ImGui::SameLine();
                }
                ImGui::NewLine();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
