#include "PhysicsSystem.h"
#include <iostream>
#include <cstdarg>
#include <stdio.h>
#include <algorithm>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

JPH_SUPPRESS_WARNINGS

using namespace JPH;

// Layer setup
namespace Layers {
    static constexpr ObjectLayer NON_MOVING = 0;
    static constexpr ObjectLayer MOVING = 1;
    static constexpr ObjectLayer NUM_LAYERS = 2;
};

class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
public:
    BPLayerInterfaceImpl() {
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayer(0);
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayer(1);
    }
    virtual uint GetNumBroadPhaseLayers() const override { return 2; }
    virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
        return mObjectToBroadPhase[inLayer];
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
        return inLayer.GetValue() == 0 ? "NON_MOVING" : "MOVING";
    }
#endif
private:
    BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayer(1);
            case Layers::MOVING: return true;
            default: return false;
        }
    }
};

class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(ObjectLayer inLayer1, ObjectLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING: return inLayer2 == Layers::MOVING;
            case Layers::MOVING: return true;
            default: return false;
        }
    }
};

BPLayerInterfaceImpl broad_phase_layer_interface;
ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
ObjectLayerPairFilterImpl object_vs_object_layer_filter;

static void TraceImpl(const char *inFMT, ...) {
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);
    std::cout << buffer << std::endl;
}

PhysEngine::PhysEngine() {}

PhysEngine::~PhysEngine() {
    for (auto b : bodies) {
        if (b && b->bodyId && physicsSystem) {
            physicsSystem->GetBodyInterface().RemoveBody(*b->bodyId);
            physicsSystem->GetBodyInterface().DestroyBody(*b->bodyId);
            delete b->bodyId;
        }
        delete b;
    }
    bodies.clear();
    
    delete physicsSystem;
    delete jobSystem;
    delete tempAllocator;
    UnregisterTypes();
    if (Factory::sInstance) {
        delete Factory::sInstance;
        Factory::sInstance = nullptr;
    }
}

void PhysEngine::init() {
    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = [](const char *inExpression, const char *inMessage, const char *inFile, uint inLine) { return true; };)
    
    Factory::sInstance = new Factory();
    RegisterTypes();
    
    tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
    jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    
    const uint cMaxBodies = 4096;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 4096;
    const uint cMaxContactConstraints = 4096;
    
    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
        broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
    
    // Set default gravity
    physicsSystem->SetGravity(Vec3(currentGravity.x, currentGravity.y, currentGravity.z));
}

void PhysEngine::update(float deltaTime) {
    if (!physicsSystem) return;
    const int cCollisionSteps = 1;
    physicsSystem->Update(deltaTime, cCollisionSteps, tempAllocator, jobSystem);
}

// Full-featured createBody with all physics parameters
PhysicsBodyData* PhysEngine::createBody(ColliderType type, glm::vec3 position, glm::vec3 scale,
                                         BodyMotionType motionType, float mass,
                                         float friction, float restitution) {
    if (!physicsSystem) return nullptr;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    
    ShapeRefC shape;
    if (type == ColliderType::BOX) {
        BoxShapeSettings settings(Vec3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));
        shape = settings.Create().Get();
    } else if (type == ColliderType::SPHERE) {
        float radius = std::max({scale.x, scale.y, scale.z}) * 0.5f;
        SphereShapeSettings settings(radius);
        shape = settings.Create().Get();
    } else if (type == ColliderType::CAPSULE) {
        float radius = std::max(scale.x, scale.z) * 0.5f;
        float halfHeight = std::max(0.01f, scale.y * 0.5f - radius);
        CapsuleShapeSettings settings(halfHeight, radius);
        shape = settings.Create().Get();
    } else if (type == ColliderType::PLANE) {
        BoxShapeSettings settings(Vec3(scale.x * 0.5f, 0.1f, scale.z * 0.5f));
        shape = settings.Create().Get();
    }
    
    RVec3 jphPos(position.x, position.y, position.z);
    
    // Map BodyMotionType to Jolt types
    EMotionType joltMotionType;
    ObjectLayer layer;
    switch (motionType) {
        case BodyMotionType::STATIC:
            joltMotionType = EMotionType::Static;
            layer = Layers::NON_MOVING;
            break;
        case BodyMotionType::KINEMATIC:
            joltMotionType = EMotionType::Kinematic;
            layer = Layers::MOVING;
            break;
        case BodyMotionType::DYNAMIC:
        default:
            joltMotionType = EMotionType::Dynamic;
            layer = Layers::MOVING;
            break;
    }
    
    BodyCreationSettings settings(shape, jphPos, Quat::sIdentity(), joltMotionType, layer);
    settings.mFriction = friction;
    settings.mRestitution = restitution;
    
    // Set mass for dynamic bodies
    if (motionType == BodyMotionType::DYNAMIC) {
        settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = mass;
    }
    
    Body* body = body_interface.CreateBody(settings);
    if (!body) return nullptr;
    
    bool shouldActivate = (motionType == BodyMotionType::DYNAMIC);
    body_interface.AddBody(body->GetID(), shouldActivate ? EActivation::Activate : EActivation::DontActivate);
    
    PhysicsBodyData* data = new PhysicsBodyData();
    data->bodyId = new BodyID(body->GetID());
    data->colliderType = type;
    data->motionType = motionType;
    bodies.push_back(data);
    return data;
}

// Legacy overload for backward compatibility
PhysicsBodyData* PhysEngine::createBody(ColliderType type, glm::vec3 position, glm::vec3 scale, bool isDynamic) {
    return createBody(type, position, scale,
                      isDynamic ? BodyMotionType::DYNAMIC : BodyMotionType::STATIC,
                      1.0f, 0.5f, 0.3f);
}

glm::vec3 PhysEngine::getBodyPosition(PhysicsBodyData* bodyData) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return glm::vec3(0.0f);
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    RVec3 pos = body_interface.GetPosition(*bodyData->bodyId);
    return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
}

void PhysEngine::setBodyPosition(PhysicsBodyData* bodyData, glm::vec3 position) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.SetPosition(*bodyData->bodyId, RVec3(position.x, position.y, position.z), EActivation::Activate);
}

glm::quat PhysEngine::getBodyRotation(PhysicsBodyData* bodyData) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return glm::quat(1, 0, 0, 0);
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    Quat q = body_interface.GetRotation(*bodyData->bodyId);
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}

void PhysEngine::setBodyRotation(PhysicsBodyData* bodyData, glm::quat rotation) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.SetRotation(*bodyData->bodyId, Quat(rotation.x, rotation.y, rotation.z, rotation.w), EActivation::Activate);
}

// ---- Force & Impulse API ----

void PhysEngine::addForce(PhysicsBodyData* bodyData, glm::vec3 force) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.AddForce(*bodyData->bodyId, Vec3(force.x, force.y, force.z));
}

void PhysEngine::addImpulse(PhysicsBodyData* bodyData, glm::vec3 impulse) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.AddImpulse(*bodyData->bodyId, Vec3(impulse.x, impulse.y, impulse.z));
}

void PhysEngine::setLinearVelocity(PhysicsBodyData* bodyData, glm::vec3 vel) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.SetLinearVelocity(*bodyData->bodyId, Vec3(vel.x, vel.y, vel.z));
}

glm::vec3 PhysEngine::getLinearVelocity(PhysicsBodyData* bodyData) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return glm::vec3(0.0f);
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    Vec3 v = body_interface.GetLinearVelocity(*bodyData->bodyId);
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

void PhysEngine::setAngularVelocity(PhysicsBodyData* bodyData, glm::vec3 vel) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.SetAngularVelocity(*bodyData->bodyId, Vec3(vel.x, vel.y, vel.z));
}

glm::vec3 PhysEngine::getAngularVelocity(PhysicsBodyData* bodyData) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return glm::vec3(0.0f);
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    Vec3 v = body_interface.GetAngularVelocity(*bodyData->bodyId);
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

// ---- Kinematic Movement ----

void PhysEngine::moveKinematic(PhysicsBodyData* bodyData, glm::vec3 targetPos, glm::quat targetRot, float dt) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.MoveKinematic(*bodyData->bodyId,
        RVec3(targetPos.x, targetPos.y, targetPos.z),
        Quat(targetRot.x, targetRot.y, targetRot.z, targetRot.w),
        dt);
}

// ---- Body Management ----

void PhysEngine::removeBody(PhysicsBodyData* bodyData) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.RemoveBody(*bodyData->bodyId);
    body_interface.DestroyBody(*bodyData->bodyId);
    bodies.erase(std::remove(bodies.begin(), bodies.end(), bodyData), bodies.end());
    delete bodyData->bodyId;
    bodyData->bodyId = nullptr;
    delete bodyData;
}

// ---- Gravity Control ----

void PhysEngine::setGravity(glm::vec3 gravity) {
    currentGravity = gravity;
    if (physicsSystem) {
        physicsSystem->SetGravity(Vec3(gravity.x, gravity.y, gravity.z));
    }
}

glm::vec3 PhysEngine::getGravity() const {
    return currentGravity;
}

// ---- Body Activation ----

void PhysEngine::activateBody(PhysicsBodyData* bodyData) {
    if (!bodyData || !bodyData->bodyId || !physicsSystem) return;
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.ActivateBody(*bodyData->bodyId);
}
