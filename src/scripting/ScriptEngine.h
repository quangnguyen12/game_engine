#pragma once

#include <sol/sol.hpp>
#include <GLFW/glfw3.h>
#include <string>

class Scene;
class PhysEngine;

class ScriptEngine
{
public:
    ScriptEngine();
    ~ScriptEngine();

    void init(GLFWwindow* window, Scene* scene, int* gameScore, int* highScore);
    void reloadLuaScripts(Scene* scene);
    void update(Scene* scene, PhysEngine* physEngine, float deltaTime);

    sol::state& getState() { return luaState; }

private:
    sol::state luaState;
    GLFWwindow* window = nullptr;
    int* pGameScore = nullptr;
    int* pHighScore = nullptr;
};
