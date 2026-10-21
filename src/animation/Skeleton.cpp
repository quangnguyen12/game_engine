#include "animation/Skeleton.h"
#include <algorithm>

std::string Skeleton::normalizeBoneName(const std::string& rawName)
{
    std::string s = rawName;
    size_t colonPos = s.find_last_of(':');
    if (colonPos != std::string::npos) {
        s = s.substr(colonPos + 1);
    }
    size_t pipePos = s.find_last_of('|');
    if (pipePos != std::string::npos) {
        s = s.substr(pipePos + 1);
    }
    std::string lower = s;
    for (char& c : lower) c = static_cast<char>(tolower(c));
    return lower;
}

int Skeleton::addBone(const std::string& name, int parentIndex, const glm::mat4& invBind, const glm::mat4& localTrans)
{
    int index = static_cast<int>(bones.size());
    Bone b;
    b.id = index;
    b.name = name;
    b.parentIndex = parentIndex;
    b.inverseBindMatrix = invBind;
    b.localTransform = localTrans;

    if (parentIndex >= 0 && parentIndex < static_cast<int>(bones.size())) {
        bones[parentIndex].children.push_back(index);
    }

    bones.push_back(b);
    boneNameToIndex[name] = index;
    normalizedNameToIndex[normalizeBoneName(name)] = index;
    return index;
}

int Skeleton::findBoneIndex(const std::string& name) const
{
    auto it = boneNameToIndex.find(name);
    if (it != boneNameToIndex.end()) return it->second;

    auto itNorm = normalizedNameToIndex.find(normalizeBoneName(name));
    if (itNorm != normalizedNameToIndex.end()) return itNorm->second;

    return -1;
}
