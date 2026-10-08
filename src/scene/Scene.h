#pragma once

#include "scene/SceneObject.h"
#include <vector>
#include <string>

class PhysEngine;

class Scene
{
public:
    Scene();

    std::vector<SceneObject>& getObjects() { return sceneObjects; }
    const std::vector<SceneObject>& getObjects() const { return sceneObjects; }

    int getSelectedObjectIndex() const { return selectedObjectIndex; }
    void setSelectedObjectIndex(int index) { selectedObjectIndex = index; }

    SceneObject* getSelectedObject();
    const SceneObject* getSelectedObject() const;

    glm::mat4 getWorldMatrix(int index) const;
    static glm::mat4 calculateWorldMatrix(const std::vector<SceneObject>& objects, int index);

    void saveHistory();
    void undo();
    void redo();

    void savePlayModeState();
    void restoreEditModeState(PhysEngine* physEngine);

    void saveScene(const std::string& filename);
    void loadScene(const std::string& filename, int cubeMeshId, int sphereMeshId, int planeMeshId);

    void initializeDefaultScene(int cubeMeshId, int sphereMeshId, int planeMeshId, int groundTexId, PhysEngine* physEngine);

    glm::vec3 getPlayerStartPos() const { return playerStartPos; }
    void setPlayerStartPos(const glm::vec3& pos) { playerStartPos = pos; }

    void initPhysicsBodies(PhysEngine* physEngine);
    void syncPhysicsToTransform(PhysEngine* physEngine);

private:
    std::vector<SceneObject> sceneObjects;
    int selectedObjectIndex = 0;

    std::vector<std::vector<SceneObject>> undoStack;
    std::vector<std::vector<SceneObject>> redoStack;

    glm::vec3 playerStartPos = glm::vec3(0.0f);
};
