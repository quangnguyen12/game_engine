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
    PLANE // Usually a large static box in Jolt
};

struct PhysicsBodyData {
    JPH::BodyID* bodyId;
};

class PhysEngine {
public:
    PhysEngine();
    ~PhysEngine();

    void init();
    void update(float deltaTime);

    // Create a rigid body. Returns a pointer to BodyID wrapper
    PhysicsBodyData* createBody(ColliderType type, glm::vec3 position, glm::vec3 scale, bool isDynamic);
    
    // Get/Set transform
    glm::vec3 getBodyPosition(PhysicsBodyData* bodyData);
    void setBodyPosition(PhysicsBodyData* bodyData, glm::vec3 position);
    glm::quat getBodyRotation(PhysicsBodyData* bodyData);

private:
    JPH::PhysicsSystem* physicsSystem = nullptr;
    JPH::TempAllocatorImpl* tempAllocator = nullptr;
    JPH::JobSystemThreadPool* jobSystem = nullptr;
    
    std::vector<PhysicsBodyData*> bodies;
};
