#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct Bone
{
    int id = -1;
    std::string name;
    int parentIndex = -1;
    std::vector<int> children;

    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
    glm::mat4 localTransform = glm::mat4(1.0f);     // Rest pose local
};

class Skeleton
{
public:
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;
    std::unordered_map<std::string, int> normalizedNameToIndex;
    glm::mat4 globalInverseTransform = glm::mat4(1.0f);
    float scaleFactor = 1.0f;

    static std::string normalizeBoneName(const std::string& rawName);

    int addBone(const std::string& name, int parentIndex, const glm::mat4& invBind, const glm::mat4& localTrans);
    int findBoneIndex(const std::string& name) const;

    size_t getBoneCount() const { return bones.size(); }
    const Bone* getBone(int index) const {
        if (index >= 0 && index < static_cast<int>(bones.size())) return &bones[index];
        return nullptr;
    }
};
