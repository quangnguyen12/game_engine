#include "animation/AnimatorComponent.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

AnimatorComponent::AnimatorComponent()
{
    type = ComponentType::ANIMATOR;
    finalBoneMatrices.assign(MAX_BONES, glm::mat4(1.0f));
    boneWorldTransforms.assign(MAX_BONES, glm::mat4(1.0f));
}

void AnimatorComponent::addClip(std::shared_ptr<AnimationClip> clip)
{
    if (!clip || clip->name.empty()) return;

    if (clips.find(clip->name) == clips.end()) {
        clipNames.push_back(clip->name);
    }
    clips[clip->name] = clip;

    if (currentClipName.empty()) {
        play(clip->name, true);
    }
}

std::shared_ptr<AnimationClip> AnimatorComponent::getClip(const std::string& name) const
{
    auto it = clips.find(name);
    if (it != clips.end()) return it->second;
    return nullptr;
}

void AnimatorComponent::play(const std::string& name, bool loop)
{
    auto it = clips.find(name);
    if (it == clips.end()) return;

    currentClipName = name;
    currentTime = 0.0f;
    isPlaying = true;
    isLooping = loop;
    isBlending = false;
}

void AnimatorComponent::crossFade(const std::string& name, float duration, bool loop)
{
    if (name == currentClipName || clips.find(name) == clips.end()) return;

    prevClipName = currentClipName;
    prevTime = currentTime;
    blendDuration = std::max(duration, 0.01f);
    blendTimer = 0.0f;
    isBlending = true;

    currentClipName = name;
    currentTime = 0.0f;
    isPlaying = true;
    isLooping = loop;
}

void AnimatorComponent::seek(float time)
{
    auto clip = getClip(currentClipName);
    if (!clip) return;

    if (clip->duration > 0.0f) {
        currentTime = std::clamp(time, 0.0f, clip->duration);
    } else {
        currentTime = std::max(time, 0.0f);
    }
}

void AnimatorComponent::onHit()
{
    if (isShootingTargetMode) {
        targetState = ShootingTargetState::HIT_FALL;
        targetStateTimer = 0.0f;
        if (clips.count("Target_Fall")) {
            play("Target_Fall", false);
        } else if (clips.count("Target_Hit_Cycle")) {
            play("Target_Hit_Cycle", false);
        }
    } else {
        // Human character reaction: crossFade to Hit Reaction if available
        if (clips.count("Hit_Reaction") || clips.count("hit_reaction")) {
            std::string hitName = clips.count("Hit_Reaction") ? "Hit_Reaction" : "hit_reaction";
            crossFade(hitName, 0.1f, false);
        }
    }
}

void AnimatorComponent::resetTarget()
{
    if (isShootingTargetMode) {
        targetState = ShootingTargetState::RESET_RISE;
        targetStateTimer = 0.0f;
        if (clips.count("Target_Rise")) {
            play("Target_Rise", false);
        } else if (clips.count("Target_Idle_Up")) {
            play("Target_Idle_Up", true);
        }
    }
}

void AnimatorComponent::update(float dt)
{
    if (!skeleton) return;

    auto currentClip = getClip(currentClipName);
    auto prevClip = isBlending ? getClip(prevClipName) : nullptr;

    // 1. Advance Playback Timeline
    if (isPlaying && currentClip && currentClip->duration > 0.0f) {
        currentTime += dt * playbackSpeed;
        if (isLooping) {
            currentTime = std::fmod(currentTime, currentClip->duration);
            if (currentTime < 0.0f) currentTime += currentClip->duration;
        } else {
            if (currentTime >= currentClip->duration) {
                currentTime = currentClip->duration;
                isPlaying = false;
            }
        }
    }

    // 2. Advance Blending Timeline
    float blendAlpha = 1.0f;
    if (isBlending) {
        blendTimer += dt;
        blendAlpha = std::clamp(blendTimer / blendDuration, 0.0f, 1.0f);
        if (prevClip && prevClip->duration > 0.0f) {
            prevTime += dt * playbackSpeed;
            prevTime = std::fmod(prevTime, prevClip->duration);
        }
        if (blendAlpha >= 1.0f) {
            isBlending = false;
            prevClipName.clear();
        }
    }

    // 3. State Machine Automation for Shooting Targets
    if (isShootingTargetMode) {
        targetStateTimer += dt;
        if (targetState == ShootingTargetState::HIT_FALL) {
            if (!isPlaying || currentTime >= (currentClip ? currentClip->duration : 0.0f)) {
                targetState = ShootingTargetState::DOWN;
                targetStateTimer = 0.0f;
                if (clips.count("Target_Idle_Down")) play("Target_Idle_Down", true);
            }
        } else if (targetState == ShootingTargetState::DOWN) {
            if (targetStateTimer >= autoResetDelay) {
                targetState = ShootingTargetState::RESET_RISE;
                targetStateTimer = 0.0f;
                if (clips.count("Target_Rise")) play("Target_Rise", false);
            }
        } else if (targetState == ShootingTargetState::RESET_RISE) {
            if (!isPlaying || currentTime >= (currentClip ? currentClip->duration : 0.0f)) {
                targetState = ShootingTargetState::READY_UP;
                targetStateTimer = 0.0f;
                if (clips.count("Target_Idle_Up")) play("Target_Idle_Up", true);
            }
        }
    }

    // 4. Compute Final Bone Matrices
    size_t boneCount = skeleton->getBoneCount();
    if (finalBoneMatrices.size() < MAX_BONES) finalBoneMatrices.resize(MAX_BONES, glm::mat4(1.0f));
    if (boneWorldTransforms.size() < boneCount) boneWorldTransforms.resize(boneCount, glm::mat4(1.0f));

    for (size_t i = 0; i < boneCount; ++i) {
        const Bone& bone = skeleton->bones[i];

        glm::vec3 pos(0.0f);
        glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scl(1.0f);

        bool sampledCurrent = false;
        if (currentClip) {
            sampledCurrent = currentClip->sampleBone(bone.name, currentTime, pos, rot, scl);
        }

        if (!sampledCurrent) {
            // Decompose rest pose
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(bone.localTransform, scl, rot, pos, skew, perspective);
        }

        if (isBlending && prevClip) {
            glm::vec3 pPos(0.0f);
            glm::quat pRot(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 pScl(1.0f);

            bool sampledPrev = prevClip->sampleBone(bone.name, prevTime, pPos, pRot, pScl);
            if (!sampledPrev) {
                glm::vec3 skew;
                glm::vec4 perspective;
                glm::decompose(bone.localTransform, pScl, pRot, pPos, skew, perspective);
            }

            pos = glm::mix(pPos, pos, blendAlpha);
            rot = glm::slerp(pRot, rot, blendAlpha);
            scl = glm::mix(pScl, scl, blendAlpha);
        }

        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 R = glm::mat4_cast(rot);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scl);
        glm::mat4 localM = T * R * S;

        glm::mat4 globalM = localM;
        if (bone.parentIndex >= 0 && bone.parentIndex < static_cast<int>(i)) {
            globalM = boneWorldTransforms[bone.parentIndex] * localM;
        }

        boneWorldTransforms[i] = globalM;

        if (i < MAX_BONES) {
            finalBoneMatrices[i] = globalM * bone.inverseBindMatrix;
        }
    }
}
