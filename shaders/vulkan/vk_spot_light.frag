#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec3 fragDirection;
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

layout(push_constant) uniform Push {
    vec4 position; // ignore w
    vec4 direction; // ignore w
    vec4 color; // w is intensity
    float radius;
    float outerCutoff; // for visualization
} push;

const float M_PI = 3.1415926538;

void main() {
    // Calculate distance from center
    float dis = sqrt(dot(fragOffset, fragOffset));
    
    // Discard pixels outside the radius
    if (dis >= 1.0) {
        discard;
    }
    
    // Create a cone-shaped light visualization
    // The cone should be more intense towards the center and fade towards the edges
    float coneIntensity = 1.0 - dis;
    
    // Apply a falloff based on the cone angle for more realistic spotlight appearance
    float falloff = smoothstep(1.0, 0.0, dis);
    
    // Add some radial gradient for a more realistic cone effect
    float radialFalloff = pow(falloff, 1.5);
    
    // Combine the effects
    float alpha = radialFalloff * coneIntensity;
    
    // Use controlled brightness to avoid bloom
    vec3 finalColor = push.color.xyz * alpha * 0.6; // Reduced brightness
    
    // Ensure we don't exceed reasonable brightness levels
    finalColor = min(finalColor, vec3(0.8));
    
    outColor = vec4(finalColor, alpha);
}