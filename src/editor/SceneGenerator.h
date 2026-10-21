#pragma once

#include <glm/glm.hpp>
#include <string>

class Scene;
class AssetManager;
class GizmoSystem;

class SceneGenerator
{
public:
    SceneGenerator();

    void generate3DScene(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem = nullptr);
    void drawSceneGeneratorPanel(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem = nullptr);

    bool& getVisible() { return showSceneGeneratorPanel; }
    int getPreset() const { return genPreset; }
    void setPreset(int p) { genPreset = p; }

private:
    bool showSceneGeneratorPanel = true;
    bool genUseReal3DModels = true;
    int genPreset = 0; // 0: Tactical Outpost, 1: Forest Clearing (Biotope 2), 2: Desert, 3: Cyberpunk City, 4: Medieval Village, 5: Floating Islands, 6: 3D Maze, 7: Industrial Warehouse, 8: Urban Street (NYC Street Scene)
    int genSeed = 42;
    int genGridSize = 35;
    float genHeightScale = 5.0f;
    float genDensity = 1.0f;
    bool genIncludeTrees = true;
    bool genIncludeRocks = true;
    bool genIncludeBuildings = true;
    bool genIncludeLights = true;
    bool genClearExisting = true;
    glm::vec3 genSunColor = glm::vec3(1.0f, 0.92f, 0.75f);
    float genSunIntensity = 1.5f;

    float jmeExportNotifyTimer = 0.0f;
    std::string jmeExportStatusMsg = "";
};
