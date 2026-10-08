#pragma once

#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "physics/PhysicsSystem.h"

enum class ComponentType
{
    TRANSFORM,
    MESH_RENDERER,
    RIGIDBODY_PHYSICS,
    LUA_SCRIPT,
    LIGHT
};

class Component
{
public:
    ComponentType type;
    bool enabled = true;

    virtual ~Component() = default;
    virtual const char* getName() const = 0;
};

class TransformComponent : public Component
{
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    TransformComponent() { type = ComponentType::TRANSFORM; }
    const char* getName() const override { return "📌 Transform"; }
};

class MeshRendererComponent : public Component
{
public:
    int meshId = -1;
    int textureId = -1;
    glm::vec4 color = glm::vec4(1.0f);
    bool visible = true;

    MeshRendererComponent() { type = ComponentType::MESH_RENDERER; }
    const char* getName() const override { return "🧊 Mesh Renderer"; }
};

class RigidBodyComponent : public Component
{
public:
    bool useGravity = true;
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    float linearDrag = 0.01f;
    float angularDrag = 0.05f;
    ColliderType colliderType = ColliderType::BOX;
    BodyMotionType motionType = BodyMotionType::DYNAMIC;
    bool isTrigger = false;
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 angularVelocity = glm::vec3(0.0f);

    RigidBodyComponent() { type = ComponentType::RIGIDBODY_PHYSICS; }
    const char* getName() const override { return "⚖️ RigidBody Physics"; }
};

class LuaScriptComponent : public Component
{
public:
    std::string scriptPath = "";
    std::string scriptContent = "";

    LuaScriptComponent() { type = ComponentType::LUA_SCRIPT; }
    const char* getName() const override { return "📖 Lua Script Component"; }
};

class LightComponent : public Component
{
public:
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 0.8f, 1.0f);
    float intensity = 1.0f;

    LightComponent() { type = ComponentType::LIGHT; }
    const char* getName() const override { return "💡 Light Component"; }
};
