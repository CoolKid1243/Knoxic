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
    vec4 position; // position for visualization
    vec4 direction; // ignore w
    vec4 color; // w is intesntiy
    float radius;
} push;

const float M_PI = 3.1415926538;

void main() {
    // Calculate distance from center
    float dis = sqrt(dot(fragOffset, fragOffset));
    
    // Discard pixels outside the radius
    if (dis >= 1.0) {
        discard;
    }
    
    // Calculate angle for sun rays
    float angle = atan(fragOffset.y, fragOffset.x);
    
    // Filled center circle (the sun)
    float centerRadius = 0.35;
    float centerCircle = 0.0;
    if (dis < centerRadius) {
        centerCircle = smoothstep(centerRadius, centerRadius * 0.7, dis);
    }
    
    // Create sun rays radiating outward all around (16 rays)
    int numRays = 16;
    float rayWidth = 0.06; // Slightly wider to ensure good coverage
    float rayStartDist = centerRadius + 0.02;
    float rayLength = 0.6; // Long rays extending to the edge
    
    float sunRays = 0.0;
    for (int i = 0; i < numRays; i++) {
        float targetAngle = (float(i) / float(numRays)) * 2.0 * M_PI;
        
        // Properly calculate the minimum angle difference considering wrapping
        float angleDiff = angle - targetAngle;
        // Normalize to [-PI, PI]
        while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
        while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;
        angleDiff = abs(angleDiff);
        
        if (angleDiff < rayWidth && dis > rayStartDist && dis < rayStartDist + rayLength) {
            float rayProgress = (dis - rayStartDist) / rayLength;
            // Fade out as we go further from center, and taper based on angle
            float rayIntensity = (1.0 - rayProgress) * smoothstep(rayWidth, 0.0, angleDiff);
            sunRays = max(sunRays, rayIntensity);
        }
    }
    
    // Combine all elements
    float alpha = max(centerCircle, sunRays);
    
    // Use very low brightness to avoid bloom
    vec3 finalColor = push.color.xyz * (centerCircle * 0.25 + sunRays * 0.3);
    outColor = vec4(finalColor, alpha);
}

