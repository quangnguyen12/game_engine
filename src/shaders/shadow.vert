#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in ivec4 inBoneIDs;
layout(location = 5) in vec4 inWeights;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    vec3 lightColor;
    vec3 viewPos;
    mat4 lightSpaceMatrix;
    vec3 lightDir;
    float enableShadows;
} ubo;

layout(set = 1, binding = 0) uniform BoneBufferObject {
    mat4 bones[128];
    int isSkinned;
} boneUbo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    vec4 localPos;
    if (boneUbo.isSkinned == 1 && (inWeights.x + inWeights.y + inWeights.z + inWeights.w) > 0.001) {
        mat4 boneTransform = mat4(0.0);
        if (inBoneIDs.x >= 0 && inBoneIDs.x < 128) boneTransform += boneUbo.bones[inBoneIDs.x] * inWeights.x;
        if (inBoneIDs.y >= 0 && inBoneIDs.y < 128) boneTransform += boneUbo.bones[inBoneIDs.y] * inWeights.y;
        if (inBoneIDs.z >= 0 && inBoneIDs.z < 128) boneTransform += boneUbo.bones[inBoneIDs.z] * inWeights.z;
        if (inBoneIDs.w >= 0 && inBoneIDs.w < 128) boneTransform += boneUbo.bones[inBoneIDs.w] * inWeights.w;
        localPos = boneTransform * vec4(inPosition, 1.0);
    } else {
        localPos = vec4(inPosition, 1.0);
    }
    gl_Position = ubo.lightSpaceMatrix * push.model * localPos;
}
