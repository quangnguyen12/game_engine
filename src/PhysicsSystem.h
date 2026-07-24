#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

// Forward declarations to avoid including Jolt everywhere
namespace JPH {
    class PhysicsSystem;
    class TempAllocatorImpl;
    class JobSystemThreadPool;
    class BodyInterface;
    class BodyID;
}

enum class ColliderType {
    BOX,
    SPHERE,
    CAPSULE,
    PLANE
};

enum class BodyMotionType {
    STATIC,      // Doesn't move (walls, floor)
    KINEMATIC,   // Moved by code, not affected by physics forces
    DYNAMIC      // Fully controlled by physics simulation
};

struct PhysicsBodyData {
    JPH::BodyID* bodyId = nullptr;
    ColliderType colliderType = ColliderType::BOX;
    BodyMotionType motionType = BodyMotionType::DYNAMIC;
};

class PhysEngine {
public:
    PhysEngine();
    ~PhysEngine();

    void init();
    void update(float deltaTime);

    // Create a rigid body with full physics parameters
    PhysicsBodyData* createBody(ColliderType type, glm::vec3 position, glm::quat rotation, glm::vec3 scale,
                                 BodyMotionType motionType, float mass = 1.0f,
                                 float friction = 0.5f, float restitution = 0.3f);

    PhysicsBodyData* createBody(ColliderType type, glm::vec3 position, glm::vec3 scale,
                                 BodyMotionType motionType, float mass = 1.0f,
                                 float friction = 0.5f, float restitution = 0.3f);
    
    // Legacy overload for backward compatibility
    PhysicsBodyData* createBody(ColliderType type, glm::vec3 position, glm::vec3 scale, bool isDynamic);

    // Get/Set transform
    glm::vec3 getBodyPosition(PhysicsBodyData* bodyData);
    void setBodyPosition(PhysicsBodyData* bodyData, glm::vec3 position);
    glm::quat getBodyRotation(PhysicsBodyData* bodyData);
    void setBodyRotation(PhysicsBodyData* bodyData, glm::quat rotation);

    // Force & Impulse API
    void addForce(PhysicsBodyData* bodyData, glm::vec3 force);
    void addImpulse(PhysicsBodyData* bodyData, glm::vec3 impulse);
    void setLinearVelocity(PhysicsBodyData* bodyData, glm::vec3 vel);
    glm::vec3 getLinearVelocity(PhysicsBodyData* bodyData);
    void setAngularVelocity(PhysicsBodyData* bodyData, glm::vec3 vel);
    glm::vec3 getAngularVelocity(PhysicsBodyData* bodyData);

    // Kinematic body movement
    void moveKinematic(PhysicsBodyData* bodyData, glm::vec3 targetPos, glm::quat targetRot, float dt);

    // Remove/destroy body
    void removeBody(PhysicsBodyData* bodyData);

    // Gravity control
    void setGravity(glm::vec3 gravity);
    glm::vec3 getGravity() const;

    // Body activation
    void activateBody(PhysicsBodyData* bodyData);

private:
    JPH::PhysicsSystem* physicsSystem = nullptr;
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;
    
    glm::vec3 currentGravity = glm::vec3(0.0f, -9.81f, 0.0f);
    std::vector<PhysicsBodyData*> bodies;
};
