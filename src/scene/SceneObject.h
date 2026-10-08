#pragma once

#include "core/Types.h"
#include "scene/Components.h"
#include "physics/PhysicsSystem.h"
#include <sol/sol.hpp>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

struct SceneObject
{
    int id = -1;
    int parentId = -1;
    std::vector<int> children;

    std::string name;
    ObjectType type = ObjectType::CUBE;

    // Component storage for Hybrid ECS System
    std::vector<std::shared_ptr<Component>> components;

    // Proxy variables for backward compatibility & easy data access
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    // PBR Material Properties
    float roughness = 0.5f;
    float metallic = 0.0f;
    bool usePBR = true;
    float ambientOcclusion = 1.0f;

    // Physics variables (for play mode)
    glm::vec3 velocity = glm::vec3(0.0f);
    bool isPhysicsEnabled = false;
    std::vector<std::string> luaScripts;
    std::vector<sol::table> luaInstances;
    PhysicsBodyData* bodyData = nullptr;

    // Physics state backup (for Edit→Play→Edit restore)
    glm::vec3 savedPosition = glm::vec3(0.0f);
    glm::vec3 savedRotation = glm::vec3(0.0f);
    glm::vec3 savedScale = glm::vec3(1.0f);

    // Rendering variables
    int meshId = -1;
    int textureId = -1;

    // Component helper methods
    template<typename T>
    std::shared_ptr<T> getComponent() const
    {
        for (const auto& comp : components)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(comp))
                return casted;
        }
        return nullptr;
    }

    bool hasComponent(ComponentType compType) const
    {
        for (const auto& comp : components)
        {
            if (comp->type == compType) return true;
        }
        return false;
    }

    void removeComponent(ComponentType compType)
    {
        components.erase(
            std::remove_if(components.begin(), components.end(),
                [compType](const std::shared_ptr<Component>& c) { return c->type == compType; }),
            components.end());
    }

    void syncComponents()
    {
        if (!hasComponent(ComponentType::TRANSFORM))
        {
            auto trans = std::make_shared<TransformComponent>();
            trans->position = position;
            trans->rotation = rotation;
            trans->scale = scale;
            components.push_back(trans);
        }
        if (meshId >= 0 && !hasComponent(ComponentType::MESH_RENDERER))
        {
            auto mesh = std::make_shared<MeshRendererComponent>();
            mesh->meshId = meshId;
            mesh->textureId = textureId;
            mesh->color = color;
            components.push_back(mesh);
        }
        if (isPhysicsEnabled && !hasComponent(ComponentType::RIGIDBODY_PHYSICS))
        {
            auto rb = std::make_shared<RigidBodyComponent>();
            rb->velocity = velocity;
            components.push_back(rb);
        }
        if (!luaScripts.empty() && !hasComponent(ComponentType::LUA_SCRIPT))
        {
            auto lua = std::make_shared<LuaScriptComponent>();
            if (!luaScripts.empty()) lua->scriptContent = luaScripts[0];
            components.push_back(lua);
        }
        if (type == ObjectType::LIGHT && !hasComponent(ComponentType::LIGHT))
        {
            auto light = std::make_shared<LightComponent>();
            light->color = color;
            components.push_back(light);
        }
    }
};
