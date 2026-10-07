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

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragPos = worldPos.xyz;
    
    // Safe normal matrix calculation protecting against degenerate scale / zero determinant
    mat3 m3 = mat3(push.model);
    float det = determinant(m3);
    mat3 normalMatrix = (abs(det) > 1e-6) ? transpose(inverse(m3)) : m3;
    vec3 transformedNormal = normalMatrix * inNormal;
    fragNormal = (length(transformedNormal) > 1e-4) ? normalize(transformedNormal) : vec3(0.0, 1.0, 0.0);
}
