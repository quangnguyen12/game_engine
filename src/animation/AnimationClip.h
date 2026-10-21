#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct KeyframeVec3
{
    float time = 0.0f;
    glm::vec3 value = glm::vec3(0.0f);
};

struct KeyframeQuat
{
    float time = 0.0f;
    glm::quat value = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
};

struct BoneTrack
{
    std::string boneName;
    std::vector<KeyframeVec3> positions;
    std::vector<KeyframeQuat> rotations;
    std::vector<KeyframeVec3> scales;

    glm::vec3 interpolatePosition(float time) const;
    glm::quat interpolateRotation(float time) const;
    glm::vec3 interpolateScale(float time) const;
    glm::mat4 evaluateLocalTransform(float time) const;
};

class AnimationClip
{
public:
    std::string name;
    float duration = 0.0f;
    float frameRate = 30.0f;
    std::vector<BoneTrack> tracks;
    std::unordered_map<std::string, size_t> trackMap;

    void addTrack(const BoneTrack& track);
    bool sampleBone(const std::string& rawBoneName, float time,
                    glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale) const;
};
