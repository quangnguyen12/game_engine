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

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 pbrParams;     // x: roughness, y: metallic, z: usePBR, w: ao
    vec4 foliageParams; // x: alphaCutoff (0.0=default 0.35), y: isFoliage (1.0=cutout black bg + SSS), z: twoSided (1.0=flip normals), w: sssIntensity
} push;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

// --- PBR Cook-Torrance Helper Functions ---

// 1. Normal Distribution Function (GGX / Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001);
}

// 2. Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// 3. Fresnel Equation (Fresnel-Schlick approximation)
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 4. Shadow Calculation
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
        
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    return shadow;
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // --- 1. Alpha Cutout / Alpha Testing ---
    if (push.foliageParams.y > 0.5) {
        float cutoff = push.foliageParams.x > 0.001 ? push.foliageParams.x : 0.35;
        if (texColor.a < cutoff) {
            discard;
        }
    } else {
        if (texColor.a < 0.02) {
            discard;
        }
    }

    // --- 2. Black Mask Cutout for OBJ Foliage Leaf Cards ---
    if (push.foliageParams.y > 0.5) {
        float maxChannel = max(texColor.r, max(texColor.g, texColor.b));
        if (maxChannel < 0.08) {
            discard;
        }
    }

    vec3 albedo = pow(texColor.rgb * fragColor, vec3(2.2)); // Convert sRGB albedo to Linear space
    
    // --- 3. Two-Sided Shading for Leaves / Foliage ---
    vec3 N = (length(fragNormal) > 1e-4) ? normalize(fragNormal) : vec3(0.0, 1.0, 0.0);
    if (push.foliageParams.z > 0.5 && !gl_FrontFacing) {
        N = -N;
    }

    vec3 viewDir = ubo.viewPos - fragPos;
    vec3 V = (length(viewDir) > 1e-4) ? normalize(viewDir) : vec3(0.0, 1.0, 0.0);
    vec3 lightDir = -ubo.lightDir;
    vec3 L = (length(lightDir) > 1e-4) ? normalize(lightDir) : vec3(0.0, 1.0, 0.0);
    vec3 H = (length(V + L) > 1e-4) ? normalize(V + L) : vec3(0.0, 1.0, 0.0);

    float roughness = clamp(push.pbrParams.x, 0.05, 1.0);
    float metallic  = clamp(push.pbrParams.y, 0.0f, 1.0);
    float usePBR    = push.pbrParams.z;
    float ao        = push.pbrParams.w;

    vec3 color = vec3(0.0);

    if (usePBR > 0.5) {
        // --- Cook-Torrance BRDF PBR Model ---
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);

        // Reflectance equation
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  

        float NdotL = max(dot(N, L), 0.0);

        // Shadow factor
        float shadow = 0.0;
        if (ubo.enableShadows > 0.5) {
            vec4 fragPosLightSpace = ubo.lightSpaceMatrix * vec4(fragPos, 1.0);
            shadow = ShadowCalculation(fragPosLightSpace, N, L);
        }

        vec3 radiance = ubo.lightColor * 2.5; // Light radiance scale
        vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);

        // --- 4. Foliage Subsurface Scattering (Sunlight Transmission) ---
        if (push.foliageParams.y > 0.5) {
            float sssStrength = push.foliageParams.w > 0.001 ? push.foliageParams.w : 0.6;
            vec3 sssColor = albedo * vec3(0.55, 1.0, 0.45); // Fresh leaf transmission tint
            float backLight = max(dot(-N, L), 0.0) * (1.0 - shadow);
            Lo += sssColor * radiance * backLight * sssStrength;
        }

        // Ambient lighting with AO (boosted slightly for foliage)
        float ambientScale = (push.foliageParams.y > 0.5) ? 0.12 : 0.05;
        vec3 ambient = vec3(ambientScale) * albedo * ao;
        color = ambient + Lo;
    } else {
        // --- Legacy Blinn-Phong Model (Fallback) ---
        vec3 ambient = 0.2 * ubo.lightColor;
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * ubo.lightColor;
        
        vec3 halfwayDir = normalize(L + V);
        float spec = pow(max(dot(N, halfwayDir), 0.0), 32.0);
        vec3 specular = 0.5 * spec * ubo.lightColor;

        float shadow = 0.0;
        if (ubo.enableShadows > 0.5) {
            vec4 fragPosLightSpace = ubo.lightSpaceMatrix * vec4(fragPos, 1.0);
            shadow = ShadowCalculation(fragPosLightSpace, N, L);
        }

        // Foliage transmission in legacy mode
        if (push.foliageParams.y > 0.5) {
            float backLight = max(dot(-N, L), 0.0) * (1.0 - shadow);
            diffuse += backLight * 0.5 * ubo.lightColor;
        }

        color = (ambient + (1.0 - shadow) * (diffuse + specular)) * albedo;
    }

    // HDR Tonemapping & Gamma Correction (Linear -> sRGB)
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, texColor.a);
}
