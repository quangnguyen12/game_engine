#include "animation/AnimationClip.h"
#include "animation/Skeleton.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::vec3 BoneTrack::interpolatePosition(float time) const
{
    if (positions.empty()) return glm::vec3(0.0f);
    if (positions.size() == 1 || time <= positions.front().time) return positions.front().value;
    if (time >= positions.back().time) return positions.back().value;

    for (size_t i = 0; i < positions.size() - 1; ++i) {
        if (time >= positions[i].time && time <= positions[i + 1].time) {
            float dt = positions[i + 1].time - positions[i].time;
            float factor = (dt > 1e-6f) ? (time - positions[i].time) / dt : 0.0f;
            return glm::mix(positions[i].value, positions[i + 1].value, factor);
        }
    }
    return positions.back().value;
}

glm::quat BoneTrack::interpolateRotation(float time) const
{
    if (rotations.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (rotations.size() == 1 || time <= rotations.front().time) return rotations.front().value;
    if (time >= rotations.back().time) return rotations.back().value;

    for (size_t i = 0; i < rotations.size() - 1; ++i) {
        if (time >= rotations[i].time && time <= rotations[i + 1].time) {
            float dt = rotations[i + 1].time - rotations[i].time;
            float factor = (dt > 1e-6f) ? (time - rotations[i].time) / dt : 0.0f;
            return glm::slerp(rotations[i].value, rotations[i + 1].value, factor);
        }
    }
    return rotations.back().value;
}

glm::vec3 BoneTrack::interpolateScale(float time) const
{
    if (scales.empty()) return glm::vec3(1.0f);
    if (scales.size() == 1 || time <= scales.front().time) return scales.front().value;
    if (time >= scales.back().time) return scales.back().value;

    for (size_t i = 0; i < scales.size() - 1; ++i) {
        if (time >= scales[i].time && time <= scales[i + 1].time) {
            float dt = scales[i + 1].time - scales[i].time;
            float factor = (dt > 1e-6f) ? (time - scales[i].time) / dt : 0.0f;
            return glm::mix(scales[i].value, scales[i + 1].value, factor);
        }
    }
    return scales.back().value;
}

glm::mat4 BoneTrack::evaluateLocalTransform(float time) const
{
    glm::vec3 pos = interpolatePosition(time);
    glm::quat rot = interpolateRotation(time);
    glm::vec3 scl = interpolateScale(time);

    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    glm::mat4 R = glm::mat4_cast(rot);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scl);
    return T * R * S;
}

void AnimationClip::addTrack(const BoneTrack& track)
{
    size_t idx = tracks.size();
    tracks.push_back(track);
    std::string norm = Skeleton::normalizeBoneName(track.boneName);
    trackMap[norm] = idx;
    trackMap[track.boneName] = idx;
}

bool AnimationClip::sampleBone(const std::string& rawBoneName, float time,
                              glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale) const
{
    auto it = trackMap.find(rawBoneName);
    if (it == trackMap.end()) {
        std::string norm = Skeleton::normalizeBoneName(rawBoneName);
        it = trackMap.find(norm);
    }
    if (it == trackMap.end()) return false;

    const BoneTrack& track = tracks[it->second];
    outPos = track.interpolatePosition(time);
    outRot = track.interpolateRotation(time);
    outScale = track.interpolateScale(time);
    return true;
}
