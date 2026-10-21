#version 450

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

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in ivec4 inBoneIDs;
layout(location = 5) in vec4 inWeights;

layout(set = 2, binding = 0) uniform BoneBufferObject {
    mat4 bones[128];
    int isSkinned;
} boneUbo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;

void main() {
    vec4 localPos;
    vec3 localNormal;

    if (boneUbo.isSkinned == 1 && (inWeights.x + inWeights.y + inWeights.z + inWeights.w) > 0.001) {
        mat4 boneTransform = mat4(0.0);
        if (inBoneIDs.x >= 0 && inBoneIDs.x < 128) boneTransform += boneUbo.bones[inBoneIDs.x] * inWeights.x;
        if (inBoneIDs.y >= 0 && inBoneIDs.y < 128) boneTransform += boneUbo.bones[inBoneIDs.y] * inWeights.y;
        if (inBoneIDs.z >= 0 && inBoneIDs.z < 128) boneTransform += boneUbo.bones[inBoneIDs.z] * inWeights.z;
        if (inBoneIDs.w >= 0 && inBoneIDs.w < 128) boneTransform += boneUbo.bones[inBoneIDs.w] * inWeights.w;

        localPos = boneTransform * vec4(inPosition, 1.0);
        localNormal = mat3(boneTransform) * inNormal;
    } else {
        localPos = vec4(inPosition, 1.0);
        localNormal = inNormal;
    }

    vec4 worldPos = push.model * localPos;
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragPos = worldPos.xyz;
    
    // Safe normal matrix calculation protecting against degenerate scale / zero determinant
    mat3 m3 = mat3(push.model);
    float det = determinant(m3);
    mat3 normalMatrix = (abs(det) > 1e-6) ? transpose(inverse(m3)) : m3;
    vec3 transformedNormal = normalMatrix * localNormal;
    fragNormal = (length(transformedNormal) > 1e-4) ? normalize(transformedNormal) : vec3(0.0, 1.0, 0.0);
}
