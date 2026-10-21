#pragma once

#include "core/Vertex.h"
#include "animation/Skeleton.h"
#include "animation/AnimationClip.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>

class AnimationLoader
{
public:
    using TextureLoaderCallback = std::function<int(const std::string&)>;
    using MemoryTextureLoaderCallback = std::function<int(const unsigned char*, size_t, const std::string&)>;

    // Load a 3D FBX model: parses geometry, submeshes, materials, armature/skinning and embedded animations
    static bool loadFBXModel(const std::string& path, Mesh& outMesh,
                             std::shared_ptr<Skeleton>& outSkeleton,
                             std::vector<std::shared_ptr<AnimationClip>>& outClips,
                             TextureLoaderCallback textureLoader = nullptr,
                             MemoryTextureLoaderCallback memTextureLoader = nullptr);

    // Load an animation-only FBX file (e.g. Mixamo motion clip) without requiring a mesh
    static bool loadFBXAnimations(const std::string& path,
                                  std::vector<std::shared_ptr<AnimationClip>>& outClips,
                                  const Skeleton* targetSkeleton = nullptr);

    // Load a single animation clip from FBX file
    static std::shared_ptr<AnimationClip> loadFBXClip(const std::string& path,
                                                      Skeleton* targetSkeleton = nullptr);

    // Fast check if an FBX file contains only animation stacks and 0 mesh vertices
    static bool isAnimationOnlyFBX(const std::string& path);
};
