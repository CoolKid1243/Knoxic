#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPosWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUv;

layout(location = 0) out vec4 outColor;

struct PointLight {
    vec4 position; // ignore w
    vec4 color; // w is intensity
};

struct SpotLight {
    vec4 position; // ignore w
    vec4 direction; // ignore w
    vec4 color; // w is intensity
    float innerCutoff; // cos of inner angle
    float outerCutoff; // cos of outer angle
    float padding1;
    float padding2;
};

struct DirectionalLight {
    vec4 direction; // ignore w
    vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLights[1000];
    int numLights;
    int numSpotLights;
    int numDirectionalLights;
    int padding;
    SpotLight spotLights[1000];
    DirectionalLight directionalLights[1000];
} ubo;

// Material textures
layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D metallicTexture;

layout(push_constant) uniform Push {
    mat4 modelMatrix;
    mat4 normalMatrix;
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec2 textureOffset;
    vec2 textureScale;
    vec3 emissionColor;
    float emissionStrength;
} push;

// Calculate tangent-bitangent-normal matrix for normal mapping
mat3 calculateTBN(vec3 normal, vec3 pos, vec2 uv) {
    vec3 dp1 = dFdx(pos);
    vec3 dp2 = dFdy(pos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    
    vec3 dp2perp = cross(dp2, normal);
    vec3 dp1perp = cross(normal, dp1);
    vec3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 bitangent = dp2perp * duv1.y + dp1perp * duv2.y;
    
    float invmax = inversesqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));
    return mat3(tangent * invmax, bitangent * invmax, normal);
}

void main() {
    // Sample albedo texture and combine with material color
    vec3 materialColor = push.albedo * texture(albedoTexture, fragUv).rgb;
    
    // Sample normal map
    vec3 normalMap = texture(normalTexture, fragUv).rgb;
    vec3 N = normalize(fragNormalWorld);
    vec3 surfaceNormal = N;

    // Use normal map if not a plain white texture
    vec3 whiteTexel = vec3(1.0, 1.0, 1.0);
    if (length(normalMap - whiteTexel) > 0.01) {
        normalMap = normalize(normalMap * 2.0 - 1.0);
        mat3 TBN = calculateTBN(N, fragPosWorld, fragUv);
        surfaceNormal = normalize(TBN * normalMap);
    }

    // Ambient + diffuse + specular
    vec3 diffuseLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
    vec3 specularLight = vec3(0.0);

    vec3 cameraPosWorld = ubo.inverseView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - fragPosWorld);

    // Point lights
    for (int i = 0; i < ubo.numLights; i++) {
        PointLight light = ubo.pointLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        directionToLight = normalize(directionToLight);

        float cosAngleIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation;

        diffuseLight += intensity * cosAngleIncidence;

        // Specular
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = clamp(dot(surfaceNormal, halfAngle), 0, 1);
        float shininess = mix(128.0, 8.0, push.roughness);
        blinnTerm = pow(blinnTerm, shininess);

        vec3 specularColor = mix(vec3(0.04), materialColor, push.metallic);
        specularLight += intensity * blinnTerm * specularColor;
    }

    // Spot lights
    for (int i = 0; i < ubo.numSpotLights; i++) {
        SpotLight light = ubo.spotLights[i];
        vec3 directionToLight = light.position.xyz - fragPosWorld;
        float distance = length(directionToLight);
        float attenuation = 1.0 / (distance * distance);
        directionToLight = normalize(directionToLight);

        // Calculate spotlight cone
        float theta = dot(directionToLight, normalize(-light.direction.xyz));
        float epsilon = light.innerCutoff - light.outerCutoff;
        float spotIntensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

        float cosAngleIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w * attenuation * spotIntensity;

        diffuseLight += intensity * cosAngleIncidence;

        // Specular
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = clamp(dot(surfaceNormal, halfAngle), 0, 1);
        float shininess = mix(128.0, 8.0, push.roughness);
        blinnTerm = pow(blinnTerm, shininess);

        vec3 specularColor = mix(vec3(0.04), materialColor, push.metallic);
        specularLight += intensity * blinnTerm * specularColor;
    }

    // Directional lights
    for (int i = 0; i < ubo.numDirectionalLights; i++) {
        DirectionalLight light = ubo.directionalLights[i];
        vec3 directionToLight = normalize(-light.direction.xyz);

        float cosAngleIncidence = max(dot(surfaceNormal, directionToLight), 0);
        vec3 intensity = light.color.xyz * light.color.w;

        diffuseLight += intensity * cosAngleIncidence;

        // Specular
        vec3 halfAngle = normalize(directionToLight + viewDirection);
        float blinnTerm = clamp(dot(surfaceNormal, halfAngle), 0, 1);
        float shininess = mix(128.0, 8.0, push.roughness);
        blinnTerm = pow(blinnTerm, shininess);

        vec3 specularColor = mix(vec3(0.04), materialColor, push.metallic);
        specularLight += intensity * blinnTerm * specularColor;
    }

    // Combine lighting
    vec3 lighting = diffuseLight * materialColor + specularLight;
    
    vec3 emission = push.emissionColor * push.emissionStrength;

    vec3 finalColor = lighting + emission;
    outColor = vec4(finalColor, 1.0);
}