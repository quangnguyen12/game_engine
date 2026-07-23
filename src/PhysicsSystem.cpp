#include "PhysicsSystem.h"
#include <iostream>
#include <cstdarg>
#include <stdio.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
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
        physicsSystem->GetBodyInterface().RemoveBody(*b->bodyId);
        physicsSystem->GetBodyInterface().DestroyBody(*b->bodyId);
        delete b->bodyId;
        delete b;
    }
    
    delete physicsSystem;
    delete jobSystem;
    delete tempAllocator;
    UnregisterTypes();
    delete Factory::sInstance;
    Factory::sInstance = nullptr;
}

void PhysEngine::init() {
    RegisterDefaultAllocator();
    Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(AssertFailed = [](const char *inExpression, const char *inMessage, const char *inFile, uint inLine) { return true; };)
    
    Factory::sInstance = new Factory();
    RegisterTypes();
    
    tempAllocator = new TempAllocatorImpl(10 * 1024 * 1024);
    jobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);
    
    const uint cMaxBodies = 1024;
    const uint cNumBodyMutexes = 0;
    const uint cMaxBodyPairs = 1024;
    const uint cMaxContactConstraints = 1024;
    
    physicsSystem = new JPH::PhysicsSystem();
    physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
        broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);
}

void PhysEngine::update(float deltaTime) {
    if (!physicsSystem) return;
    const int cCollisionSteps = 1;
    physicsSystem->Update(deltaTime, cCollisionSteps, tempAllocator, jobSystem);
}

PhysicsBodyData* PhysEngine::createBody(ColliderType type, glm::vec3 position, glm::vec3 scale, bool isDynamic) {
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    
    ShapeRefC shape;
    if (type == ColliderType::BOX) {
        BoxShapeSettings settings(Vec3(scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f));
        shape = settings.Create().Get();
    } else if (type == ColliderType::SPHERE) {
        SphereShapeSettings settings(scale.x * 0.5f);
        shape = settings.Create().Get();
    } else if (type == ColliderType::PLANE) {
        BoxShapeSettings settings(Vec3(scale.x * 0.5f, 0.1f, scale.z * 0.5f));
        shape = settings.Create().Get();
    }
    RVec3 jphPos(position.x, position.y, position.z);
    
    ObjectLayer layer = isDynamic ? Layers::MOVING : Layers::NON_MOVING;
    EMotionType motionType = isDynamic ? EMotionType::Dynamic : EMotionType::Static;
    
    BodyCreationSettings settings(shape, jphPos, Quat::sIdentity(), motionType, layer);
    
    Body* body = body_interface.CreateBody(settings);
    body_interface.AddBody(body->GetID(), isDynamic ? EActivation::Activate : EActivation::DontActivate);
    
    PhysicsBodyData* data = new PhysicsBodyData();
    data->bodyId = new BodyID(body->GetID());
    bodies.push_back(data);
    return data;
}

glm::vec3 PhysEngine::getBodyPosition(PhysicsBodyData* bodyData) {
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    RVec3 pos = body_interface.GetPosition(*bodyData->bodyId);
    return glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
}

void PhysEngine::setBodyPosition(PhysicsBodyData* bodyData, glm::vec3 position) {
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    body_interface.SetPosition(*bodyData->bodyId, RVec3(position.x, position.y, position.z), EActivation::DontActivate);
}

glm::quat PhysEngine::getBodyRotation(PhysicsBodyData* bodyData) {
    BodyInterface& body_interface = physicsSystem->GetBodyInterface();
    Quat q = body_interface.GetRotation(*bodyData->bodyId);
    return glm::quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
}
