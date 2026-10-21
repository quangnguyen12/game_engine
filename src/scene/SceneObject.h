#pragma once

#include "core/Types.h"
#include "core/Vertex.h"
#include "scene/Components.h"
#include "animation/Skeleton.h"
#include "animation/AnimationClip.h"
#include "animation/AnimatorComponent.h"
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
    std::string parentBoneName = "";      // Primary bone (e.g. "CC_Base_R_Hand" - Grip / Trigger)
    std::string secondaryBoneName = "";   // Secondary bone (e.g. "CC_Base_L_Hand" - Handguard / Muzzle)
    bool enableTwoHandedAiming = false;   // Dynamic 2-handed lock (barrel points at secondary bone)
    bool enableDynamicWeaponSockets = true; // Auto-switch between Hand & Chest based on animation (Reload -> R_Hand, Grenade -> Chest)
    std::string chestBoneName = "CC_Base_Spine02"; // Chest bone for grenade toss / slung carry
    glm::vec3 chestOffsetPosition = glm::vec3(0.05f, 0.05f, 0.16f);
    glm::vec3 chestOffsetRotation = glm::vec3(45.0f, -30.0f, 135.0f);
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
    std::string modelPath = "";
    std::string texturePath = "";

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

    std::shared_ptr<AnimatorComponent> getAnimator() const
    {
        return getComponent<AnimatorComponent>();
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
        if (auto trans = getComponent<TransformComponent>())
        {
            trans->position = position;
            trans->rotation = rotation;
            trans->scale = scale;
        }
        else
        {
            auto newTrans = std::make_shared<TransformComponent>();
            newTrans->position = position;
            newTrans->rotation = rotation;
            newTrans->scale = scale;
            components.push_back(newTrans);
        }

        if (auto mesh = getComponent<MeshRendererComponent>())
        {
            mesh->meshId = meshId;
            mesh->textureId = textureId;
            mesh->color = color;
        }
        else if (meshId >= 0)
        {
            auto newMesh = std::make_shared<MeshRendererComponent>();
            newMesh->meshId = meshId;
            newMesh->textureId = textureId;
            newMesh->color = color;
            components.push_back(newMesh);
        }

        if (auto rb = getComponent<RigidBodyComponent>())
        {
            rb->velocity = velocity;
        }
        else if (isPhysicsEnabled)
        {
            auto newRb = std::make_shared<RigidBodyComponent>();
            newRb->velocity = velocity;
            components.push_back(newRb);
        }

        if (auto lua = getComponent<LuaScriptComponent>())
        {
            if (!luaScripts.empty()) lua->scriptContent = luaScripts[0];
        }
        else if (!luaScripts.empty())
        {
            auto newLua = std::make_shared<LuaScriptComponent>();
            newLua->scriptContent = luaScripts[0];
            components.push_back(newLua);
        }

        if (auto light = getComponent<LightComponent>())
        {
            light->color = color;
        }
        else if (type == ObjectType::LIGHT)
        {
            auto newLight = std::make_shared<LightComponent>();
            newLight->color = color;
            components.push_back(newLight);
        }
    }

    void autoAttachAnimatorIfSkinned(const std::vector<Mesh>& meshes)
    {
        if (meshId >= 0 && meshId < static_cast<int>(meshes.size()))
        {
            const auto& m = meshes[meshId];
            if (m.isSkinned && m.skeleton)
            {
                auto animComp = getAnimator();
                if (!animComp)
                {
                    animComp = std::make_shared<AnimatorComponent>();
                    components.push_back(animComp);
                }
                animComp->skeleton = m.skeleton;
                for (const auto& clip : m.defaultClips)
                {
                    if (clip) {
                        animComp->addClip(clip);
                    }
                }
                if (!m.defaultClips.empty() && !animComp->isPlaying)
                {
                    animComp->play(m.defaultClips[0]->name, true);
                }
            }
        }
    }
};
