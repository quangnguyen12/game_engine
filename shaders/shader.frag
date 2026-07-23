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

layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;


float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    // Check if outside light frustum
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
        
    float currentDepth = projCoords.z;
    
    // Calculate bias (based on depth map resolution and slope)
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    // 1. Texture/Base color
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 objectColor = texColor.rgb * fragColor;
    
    // 2. Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * ubo.lightColor;
    
    // 3. Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(-ubo.lightDir); // Directional light model
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;
    
    // 4. Specular (Blinn-Phong)
    float specularStrength = 0.5;
    vec3 viewDir = normalize(ubo.viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * ubo.lightColor;
    

    // 5. Shadow
    float shadow = 0.0;
    if (ubo.enableShadows > 0.5) {
        vec4 fragPosLightSpace = ubo.lightSpaceMatrix * vec4(fragPos, 1.0);
        shadow = ShadowCalculation(fragPosLightSpace, norm, lightDir);
    }
    
    vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;
    outColor = vec4(result, texColor.a);
}
