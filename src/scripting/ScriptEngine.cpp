#include "scripting/ScriptEngine.h"
#include "scene/Scene.h"
#include "physics/PhysicsSystem.h"
#include <iostream>

ScriptEngine::ScriptEngine()
{
}

ScriptEngine::~ScriptEngine()
{
}

void ScriptEngine::init(GLFWwindow* win, Scene* scene, int* gameScore, int* highScore)
{
    window = win;
    pGameScore = gameScore;
    pHighScore = highScore;

    luaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

    // Bind SceneObject to Lua
    luaState.new_usertype<SceneObject>("SceneObject",
        "name", &SceneObject::name,
        "position", &SceneObject::position,
        "rotation", &SceneObject::rotation,
        "scale", &SceneObject::scale,
        "velocity", &SceneObject::velocity
    );

    // Bind glm::vec3
    luaState.new_usertype<glm::vec3>("vec3",
        sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
        "x", &glm::vec3::x,
        "y", &glm::vec3::y,
        "z", &glm::vec3::z
    );

    // Bind Game API to Lua
    auto gameTable = luaState.create_named_table("Game");
    gameTable.set_function("addScore", [this](int points) {
        if (pGameScore) {
            *pGameScore += points;
            if (pHighScore && *pGameScore > *pHighScore) {
                *pHighScore = *pGameScore;
            }
        }
    });
    gameTable.set_function("getScore", [this]() -> int {
        return pGameScore ? *pGameScore : 0;
    });
    gameTable.set_function("getPlayerPosition", [scene]() -> glm::vec3 {
        if (!scene) return glm::vec3(0.0f);
        for (const auto& obj : scene->getObjects()) {
            if (obj.name == "Player Cube") return obj.position;
        }
        return glm::vec3(0.0f);
    });

    // Bind Input API to Lua
    auto inputTable = luaState.create_named_table("Input");
    inputTable.set_function("isKeyPressed", [this](const std::string& key) -> bool {
        if (!window) return false;
        std::string k = key;
        for (auto& c : k) c = static_cast<char>(toupper(c));

        int glfwKey = -1;
        if (k == "W") glfwKey = GLFW_KEY_W;
        else if (k == "A") glfwKey = GLFW_KEY_A;
        else if (k == "S") glfwKey = GLFW_KEY_S;
        else if (k == "D") glfwKey = GLFW_KEY_D;
        else if (k == "UP") glfwKey = GLFW_KEY_UP;
        else if (k == "DOWN") glfwKey = GLFW_KEY_DOWN;
        else if (k == "LEFT") glfwKey = GLFW_KEY_LEFT;
        else if (k == "RIGHT") glfwKey = GLFW_KEY_RIGHT;
        else if (k == "SPACE") glfwKey = GLFW_KEY_SPACE;
        else if (k == "SHIFT") glfwKey = GLFW_KEY_LEFT_SHIFT;
        else if (k == "CTRL") glfwKey = GLFW_KEY_LEFT_CONTROL;
        else if (k.length() == 1 && k[0] >= 'A' && k[0] <= 'Z') glfwKey = GLFW_KEY_A + (k[0] - 'A');
        else if (k.length() == 1 && k[0] >= '0' && k[0] <= '9') glfwKey = GLFW_KEY_0 + (k[0] - '0');

        if (glfwKey != -1) {
            return glfwGetKey(window, glfwKey) == GLFW_PRESS;
        }
        return false;
    });
}

void ScriptEngine::reloadLuaScripts(Scene* scene)
{
    if (!scene) return;
    for (auto& obj : scene->getObjects())
    {
        obj.luaInstances.clear();
        for (const auto& script : obj.luaScripts)
        {
            if (script.empty()) continue;
            try {
                sol::protected_function_result result = luaState.script(script);
                if (result.valid()) {
                    sol::table instance;
                    bool isGlobals = false;
                    if (result.get_type() == sol::type::table) {
                        instance = result;
                    } else {
                        instance = luaState.globals();
                        isGlobals = true;
                    }
                    obj.luaInstances.push_back(instance);
                    sol::protected_function onStart = instance["onStart"];
                    if (onStart.valid()) {
                        sol::protected_function_result res;
                        if (isGlobals)
                            res = onStart(&obj);
                        else
                            res = onStart(instance, &obj);

                        if (!res.valid()) {
                            sol::error err = res;
                            printf("Lua onStart error: %s\n", err.what());
                        }
                    }
                }
            } catch (const sol::error& e) {
                printf("Lua syntax error: %s\n", e.what());
            }
        }
    }
}

void ScriptEngine::update(Scene* scene, PhysEngine* physEngine, float deltaTime)
{
    if (!scene) return;
    for (auto& obj : scene->getObjects())
    {
        glm::vec3 prevPos = obj.position;
        glm::vec3 prevVel = obj.velocity;

        for (auto& luaInst : obj.luaInstances)
        {
            if (luaInst.valid())
            {
                sol::protected_function onUpdate = luaInst["onUpdate"];
                if (onUpdate.valid())
                {
                    sol::protected_function_result res;
                    if (luaInst == luaState.globals())
                        res = onUpdate(&obj, deltaTime);
                    else
                        res = onUpdate(luaInst, &obj, deltaTime);

                    if (!res.valid())
                    {
                        sol::error err = res;
                        printf("Lua onUpdate error: %s\n", err.what());
                    }
                }
            }
        }

        if (obj.bodyData && physEngine)
        {
            if (obj.position != prevPos)
            {
                physEngine->setBodyPosition(obj.bodyData, obj.position);
                physEngine->activateBody(obj.bodyData);
            }
            if (obj.velocity != prevVel)
            {
                physEngine->setLinearVelocity(obj.bodyData, obj.velocity);
                physEngine->activateBody(obj.bodyData);
            }
        }
    }
}
