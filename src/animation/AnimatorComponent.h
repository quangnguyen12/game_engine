#pragma once

#include "scene/Components.h"
#include "animation/Skeleton.h"
#include "animation/AnimationClip.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

enum class ShootingTargetState
{
    READY_UP,
    HIT_FALL,
    DOWN,
    RESET_RISE
};

class AnimatorComponent : public Component
{
public:
    static constexpr size_t MAX_BONES = 128;

    std::shared_ptr<Skeleton> skeleton;
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> clips;
    std::vector<std::string> clipNames;

    std::string currentClipName = "";
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool isPlaying = true;
    bool isLooping = true;

    // Blending (Cross-Fade)
    std::string prevClipName = "";
    float prevTime = 0.0f;
    float blendTimer = 0.0f;
    float blendDuration = 0.0f;
    bool isBlending = false;

    // Shooting Target Module Logic
    bool isShootingTargetMode = false;
    ShootingTargetState targetState = ShootingTargetState::READY_UP;
    float targetStateTimer = 0.0f;
    float autoResetDelay = 3.0f;

    // Final matrices sent to GPU Vertex Shader: F_i = M_i * invBind_i
    std::vector<glm::mat4> finalBoneMatrices;
    std::vector<glm::mat4> boneWorldTransforms;

    AnimatorComponent();
    const char* getName() const override { return "🎭 Animator"; }

    void addClip(std::shared_ptr<AnimationClip> clip);
    std::shared_ptr<AnimationClip> getClip(const std::string& name) const;
    void play(const std::string& name, bool loop = true);
    void crossFade(const std::string& name, float duration, bool loop = true);
    void pause() { isPlaying = false; }
    void resume() { isPlaying = true; }
    void stop() { isPlaying = false; currentTime = 0.0f; }
    void seek(float time);

    void update(float dt);

    // Shooting target specific trigger
    void onHit();
    void resetTarget();

private:
    void computeBoneTransforms(float dt);
};
