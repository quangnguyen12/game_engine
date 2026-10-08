#include "scene/Scene.h"
#include "physics/PhysicsSystem.h"
#include <fstream>
#include <sstream>
#include <iostream>

Scene::Scene()
    : selectedObjectIndex(0),
      playerStartPos(0.0f, 0.0f, 0.0f)
{
}

SceneObject* Scene::getSelectedObject()
{
    if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        return &sceneObjects[selectedObjectIndex];
    }
    return nullptr;
}

const SceneObject* Scene::getSelectedObject() const
{
    if (selectedObjectIndex >= 0 && selectedObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        return &sceneObjects[selectedObjectIndex];
    }
    return nullptr;
}

glm::mat4 Scene::getWorldMatrix(int index) const
{
    return calculateWorldMatrix(sceneObjects, index);
}

glm::mat4 Scene::calculateWorldMatrix(const std::vector<SceneObject>& objects, int index)
{
    if (index < 0 || index >= static_cast<int>(objects.size())) return glm::mat4(1.0f);

    const auto& obj = objects[index];
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.position);
    model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, obj.scale);

    if (obj.parentId >= 0 && obj.parentId < static_cast<int>(objects.size()) && obj.parentId != index)
    {
        return calculateWorldMatrix(objects, obj.parentId) * model;
    }

    return model;
}

void Scene::saveHistory()
{
    undoStack.push_back(sceneObjects);
    redoStack.clear();
}

void Scene::undo()
{
    if (!undoStack.empty())
    {
        redoStack.push_back(sceneObjects);
        sceneObjects = undoStack.back();
        undoStack.pop_back();
        if (selectedObjectIndex >= static_cast<int>(sceneObjects.size()))
        {
            selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
        }
    }
}

void Scene::redo()
{
    if (!redoStack.empty())
    {
        undoStack.push_back(sceneObjects);
        sceneObjects = redoStack.back();
        redoStack.pop_back();
        if (selectedObjectIndex >= static_cast<int>(sceneObjects.size()))
        {
            selectedObjectIndex = static_cast<int>(sceneObjects.size()) - 1;
        }
    }
}

void Scene::savePlayModeState()
{
    for (auto& obj : sceneObjects)
    {
        obj.savedPosition = obj.position;
        obj.savedRotation = obj.rotation;
        obj.savedScale = obj.scale;
    }
}

void Scene::restoreEditModeState(PhysEngine* physEngine)
{
    for (auto& obj : sceneObjects)
    {
        obj.position = obj.savedPosition;
        obj.rotation = obj.savedRotation;
        obj.scale = obj.savedScale;
        obj.velocity = glm::vec3(0.0f);
        if (obj.bodyData && physEngine)
        {
            physEngine->setBodyPosition(obj.bodyData, obj.position);
            glm::quat rotQuat = glm::quat(glm::radians(obj.rotation));
            physEngine->setBodyRotation(obj.bodyData, rotQuat);
            physEngine->setLinearVelocity(obj.bodyData, glm::vec3(0.0f));
            physEngine->setAngularVelocity(obj.bodyData, glm::vec3(0.0f));
        }
    }
}

void Scene::saveScene(const std::string& filename)
{
    std::ofstream out(filename);
    if (!out.is_open()) return;

    out << "# ShapeRenderer Scene File\n";
    for (const auto& obj : sceneObjects)
    {
        out << "object: " << obj.name << "\n";
        out << "type: " << static_cast<int>(obj.type) << "\n";
        out << "position: " << obj.position.x << " " << obj.position.y << " " << obj.position.z << "\n";
        out << "rotation: " << obj.rotation.x << " " << obj.rotation.y << " " << obj.rotation.z << "\n";
        out << "scale: " << obj.scale.x << " " << obj.scale.y << " " << obj.scale.z << "\n";
        out << "color: " << obj.color.r << " " << obj.color.g << " " << obj.color.b << " " << obj.color.a << "\n";
        out << "physics: " << (obj.isPhysicsEnabled ? 1 : 0) << "\n";
        
        auto rbSave = obj.getComponent<RigidBodyComponent>();
        if (rbSave)
        {
            out << "mass: " << rbSave->mass << "\n";
            out << "friction: " << rbSave->friction << "\n";
            out << "restitution: " << rbSave->restitution << "\n";
            out << "linearDrag: " << rbSave->linearDrag << "\n";
            out << "angularDrag: " << rbSave->angularDrag << "\n";
            out << "colliderType: " << static_cast<int>(rbSave->colliderType) << "\n";
            out << "motionType: " << static_cast<int>(rbSave->motionType) << "\n";
            out << "useGravity: " << (rbSave->useGravity ? 1 : 0) << "\n";
        }
        
        out << "luaScriptsCount: " << obj.luaScripts.size() << "\n";
        for (const auto& script : obj.luaScripts)
        {
            out << "luaScriptStart:\n";
            out << script;
            if (!script.empty() && script.back() != '\n') out << "\n";
            out << "luaScriptEnd:\n";
        }
        out << "\n";
    }
    out.close();
}

void Scene::loadScene(const std::string& filename, int cubeMeshId, int sphereMeshId, int planeMeshId)
{
    std::ifstream in(filename);
    if (!in.is_open()) return;

    sceneObjects.clear();
    std::string line;
    SceneObject current;
    bool hasObj = false;

    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string key;
        ss >> key;

        if (key == "object:")
        {
            if (hasObj)
            {
                sceneObjects.push_back(current);
            }
            std::string name;
            std::getline(ss, name);
            if (!name.empty() && name[0] == ' ') name = name.substr(1);
            current = SceneObject();
            current.name = name;
            hasObj = true;
        }
        else if (key == "type:")
        {
            int t;
            ss >> t;
            current.type = static_cast<ObjectType>(t);
        }
        else if (key == "position:")
        {
            ss >> current.position.x >> current.position.y >> current.position.z;
        }
        else if (key == "rotation:")
        {
            ss >> current.rotation.x >> current.rotation.y >> current.rotation.z;
        }
        else if (key == "scale:")
        {
            ss >> current.scale.x >> current.scale.y >> current.scale.z;
        }
        else if (key == "color:")
        {
            ss >> current.color.r >> current.color.g >> current.color.b >> current.color.a;
        }
        else if (key == "physics:")
        {
            int p;
            ss >> p;
            current.isPhysicsEnabled = (p == 1);
            if (current.isPhysicsEnabled)
            {
                if (!current.hasComponent(ComponentType::RIGIDBODY_PHYSICS))
                {
                    auto rb = std::make_shared<RigidBodyComponent>();
                    current.components.push_back(rb);
                }
            }
        }
        else if (key == "mass:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->mass = v;
        }
        else if (key == "friction:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->friction = v;
        }
        else if (key == "restitution:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->restitution = v;
        }
        else if (key == "linearDrag:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->linearDrag = v;
        }
        else if (key == "angularDrag:")
        {
            float v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->angularDrag = v;
        }
        else if (key == "colliderType:")
        {
            int v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->colliderType = static_cast<ColliderType>(v);
        }
        else if (key == "motionType:")
        {
            int v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->motionType = static_cast<BodyMotionType>(v);
        }
        else if (key == "useGravity:")
        {
            int v; ss >> v;
            auto rb = current.getComponent<RigidBodyComponent>();
            if (rb) rb->useGravity = (v == 1);
        }
        else if (key == "luaScriptStart:")
        {
            std::string scriptContent = "";
            std::string scriptLine;
            while (std::getline(in, scriptLine))
            {
                if (!scriptLine.empty() && scriptLine.back() == '\r') scriptLine.pop_back();
                if (scriptLine == "luaScriptEnd:") break;
                scriptContent += scriptLine + "\n";
            }
            if (!scriptContent.empty()) scriptContent.pop_back();
            current.luaScripts.push_back(scriptContent);
        }
    }
    if (hasObj)
    {
        sceneObjects.push_back(current);
    }
    in.close();

    for (auto& obj : sceneObjects)
    {
        if (obj.name == "Player Cube")
        {
            playerStartPos = obj.position;
        }
        switch (obj.type) {
            case ObjectType::CUBE:   obj.meshId = cubeMeshId; break;
            case ObjectType::SPHERE: obj.meshId = sphereMeshId; break;
            case ObjectType::PLANE:  obj.meshId = planeMeshId; break;
            case ObjectType::LIGHT:  obj.meshId = cubeMeshId; break;
            default: obj.meshId = -1; break;
        }
    }
    selectedObjectIndex = 0;
}

void Scene::initializeDefaultScene(int cubeMeshId, int sphereMeshId, int planeMeshId, int groundTexId, PhysEngine* physEngine)
{
    sceneObjects.clear();
    
    // 1. Player Cube
    SceneObject player;
    player.name = "Player Cube";
    player.type = ObjectType::CUBE;
    player.position = glm::vec3(0.0f, 0.0f, 0.0f);
    player.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    player.scale = glm::vec3(0.5f, 0.5f, 0.5f);
    player.color = glm::vec4(0.0f, 0.8f, 1.0f, 1.0f);
    player.isPhysicsEnabled = true;
    player.velocity = glm::vec3(0.0f);
    player.id = 0;
    player.meshId = cubeMeshId;

    auto playerRb = std::make_shared<RigidBodyComponent>();
    playerRb->colliderType = ColliderType::BOX;
    playerRb->motionType = BodyMotionType::DYNAMIC;
    playerRb->mass = 1.0f;
    player.components.push_back(playerRb);

    std::string playerLuaScript = R"(local PlayerWASD = {}

function PlayerWASD:onStart(obj)
    self.speed = 4.0
    print("[Lua] PlayerWASD started on: " .. obj.name)
end

function PlayerWASD:onUpdate(obj, dt)
    local moveX = 0.0
    local moveZ = 0.0

    if Input.isKeyPressed("W") or Input.isKeyPressed("UP") then moveZ = moveZ - 1.0 end
    if Input.isKeyPressed("S") or Input.isKeyPressed("DOWN") then moveZ = moveZ + 1.0 end
    if Input.isKeyPressed("A") or Input.isKeyPressed("LEFT") then moveX = moveX - 1.0 end
    if Input.isKeyPressed("D") or Input.isKeyPressed("RIGHT") then moveX = moveX + 1.0 end

    if moveX ~= 0.0 and moveZ ~= 0.0 then
        moveX = moveX * 0.7071
        moveZ = moveZ * 0.7071
    end

    obj.position.x = obj.position.x + moveX * self.speed * dt
    obj.position.z = obj.position.z + moveZ * self.speed * dt

    if moveX ~= 0.0 or moveZ ~= 0.0 then
        local angle = math.deg(math.atan(moveX, -moveZ))
        obj.rotation.y = angle
    end

    if Input.isKeyPressed("SPACE") and obj.position.y <= 0.05 then
        obj.velocity.y = 5.0
    end
end

return PlayerWASD
)";
    player.luaScripts.push_back(playerLuaScript);

    std::string respawnLuaScript = R"(local RespawnOnFall = {}

function RespawnOnFall:onStart(obj)
    self.fallThreshold = -10.0
    self.spawnPos = { x = obj.position.x, y = obj.position.y + 2.0, z = obj.position.z }
    print("[Lua] RespawnOnFall initialized for: " .. obj.name)
end

function RespawnOnFall:onUpdate(obj, dt)
    if obj.position.y < self.fallThreshold then
        print("[Lua] " .. obj.name .. " fell out of bounds! Respawning...")
        obj.position.x = self.spawnPos.x
        obj.position.y = self.spawnPos.y
        obj.position.z = self.spawnPos.z

        obj.velocity.x = 0.0
        obj.velocity.y = 0.0
        obj.velocity.z = 0.0
    end
end

return RespawnOnFall
)";
    player.luaScripts.push_back(respawnLuaScript);
    sceneObjects.push_back(player);
    playerStartPos = player.position;

    // 2. Collectible Sphere
    SceneObject target;
    target.id = 1;
    target.name = "Gold Collectible";
    target.type = ObjectType::SPHERE;
    target.position = glm::vec3(1.5f, -1.0f, 1.0f);
    target.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    target.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    target.color = glm::vec4(1.0f, 0.84f, 0.0f, 1.0f);
    target.isPhysicsEnabled = true;
    target.meshId = sphereMeshId;

    auto targetRb = std::make_shared<RigidBodyComponent>();
    targetRb->colliderType = ColliderType::SPHERE;
    targetRb->motionType = BodyMotionType::DYNAMIC;
    targetRb->mass = 0.5f;
    target.components.push_back(targetRb);

    std::string goldLuaScript = R"(local GoldCollectible = {}

function GoldCollectible:onStart(obj)
    self.pickupRadius = 0.6
    print("[Lua] GoldCollectible initialized for: " .. obj.name)
end

function GoldCollectible:onUpdate(obj, dt)
    obj.rotation.y = obj.rotation.y + 90.0 * dt
    local playerPos = Game.getPlayerPosition()

    local dx = obj.position.x - playerPos.x
    local dy = obj.position.y - playerPos.y
    local dz = obj.position.z - playerPos.z
    local dist = math.sqrt(dx * dx + dy * dy + dz * dz)

    if dist < self.pickupRadius then
        print("[Lua] Gold Collected! +1 Score")
        Game.addScore(1)
        obj.position.x = math.random(-30, 30) / 10.0
        obj.position.y = -1.2
        obj.position.z = math.random(-30, 30) / 10.0
    end
end

return GoldCollectible
)";
    target.luaScripts.push_back(goldLuaScript);
    sceneObjects.push_back(target);

    // 3. Ground Plane
    SceneObject ground;
    ground.id = 2;
    ground.name = "Ground Obstacle";
    ground.type = ObjectType::PLANE;
    ground.position = glm::vec3(0.0f, -1.5f, 0.0f);
    ground.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    ground.scale = glm::vec3(5.0f, 0.1f, 5.0f);
    ground.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ground.roughness = 0.85f;
    ground.metallic = 0.1f;
    ground.usePBR = true;
    ground.isPhysicsEnabled = true;
    ground.meshId = planeMeshId;
    ground.textureId = groundTexId;

    auto groundRb = std::make_shared<RigidBodyComponent>();
    groundRb->colliderType = ColliderType::PLANE;
    groundRb->motionType = BodyMotionType::STATIC;
    groundRb->mass = 0.0f;
    ground.components.push_back(groundRb);
    sceneObjects.push_back(ground);

    // 4. Directional Light
    SceneObject sun;
    sun.id = 3;
    sun.name = "Directional Light";
    sun.type = ObjectType::LIGHT;
    sun.position = glm::vec3(2.0f, 3.0f, 1.0f);
    sun.rotation = glm::vec3(45.0f, 45.0f, 0.0f);
    sun.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    sun.color = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);
    sun.isPhysicsEnabled = false;
    sun.meshId = cubeMeshId;
    sceneObjects.push_back(sun);

    selectedObjectIndex = 0;
    if (physEngine)
    {
        initPhysicsBodies(physEngine);
    }
    saveHistory();
}

void Scene::initPhysicsBodies(PhysEngine* physEngine)
{
    if (!physEngine) return;
    for (auto& obj : sceneObjects)
    {
        if (obj.bodyData)
        {
            physEngine->removeBody(obj.bodyData);
            obj.bodyData = nullptr;
        }

        if (obj.isPhysicsEnabled || obj.hasComponent(ComponentType::RIGIDBODY_PHYSICS))
        {
            auto rb = obj.getComponent<RigidBodyComponent>();
            if (!rb)
            {
                rb = std::make_shared<RigidBodyComponent>();
                if (obj.type == ObjectType::SPHERE) rb->colliderType = ColliderType::SPHERE;
                else if (obj.type == ObjectType::PLANE) {
                    rb->colliderType = ColliderType::PLANE;
                    rb->motionType = BodyMotionType::STATIC;
                }
                obj.components.push_back(rb);
            }

            obj.isPhysicsEnabled = true;

            ColliderType ct = rb->colliderType;
            BodyMotionType mt = rb->motionType;
            float mass = rb->mass;
            float friction = rb->friction;
            float restitution = rb->restitution;

            glm::quat rotQuat = glm::quat(glm::radians(obj.rotation));
            obj.bodyData = physEngine->createBody(ct, obj.position, rotQuat, obj.scale, mt, mass, friction, restitution);
        }
    }
}

void Scene::syncPhysicsToTransform(PhysEngine* physEngine)
{
    if (!physEngine) return;
    for (auto& obj : sceneObjects)
    {
        if (!obj.bodyData) continue;

        auto rb = obj.getComponent<RigidBodyComponent>();
        if (!rb) continue;
        if (rb->motionType == BodyMotionType::STATIC) continue;

        obj.position = physEngine->getBodyPosition(obj.bodyData);
        glm::quat q = physEngine->getBodyRotation(obj.bodyData);
        obj.rotation = glm::degrees(glm::eulerAngles(q));
        obj.velocity = physEngine->getLinearVelocity(obj.bodyData);

        if (rb->linearDrag > 0.0f)
        {
            glm::vec3 vel = obj.velocity;
            vel *= (1.0f - rb->linearDrag);
            physEngine->setLinearVelocity(obj.bodyData, vel);
        }

        if (rb->angularDrag > 0.0f)
        {
            glm::vec3 avel = physEngine->getAngularVelocity(obj.bodyData);
            avel *= (1.0f - rb->angularDrag);
            physEngine->setAngularVelocity(obj.bodyData, avel);
        }
    }
}
