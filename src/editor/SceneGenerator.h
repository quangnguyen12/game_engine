#pragma once

#include <glm/glm.hpp>

class Scene;
class AssetManager;
class GizmoSystem;

class SceneGenerator
{
public:
    SceneGenerator();

    void generate3DScene(Scene* scene, AssetManager* assetManager);
    void drawSceneGeneratorPanel(Scene* scene, AssetManager* assetManager, GizmoSystem* gizmoSystem = nullptr);

    bool& getVisible() { return showSceneGeneratorPanel; }

private:
    bool showSceneGeneratorPanel = true;
    bool genUseReal3DModels = true;
    int genPreset = 0; // 0: Forest, 1: Desert, 2: Cyberpunk City, 3: Medieval Village, 4: Floating Islands, 5: 3D Maze
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
};
