#version 450

layout(location = 0) in vec2 fragOffset;
layout(location = 0) out vec4 outColor;

struct PointLight {
    vec4 position; // ignore w
    vec4 color; // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUbo {
    mat4 projection;
    mat4 view;
    mat4 inverseView;
    vec4 ambientLightColor; // w is intensity
    PointLight pointLights[1000];
    int numLights;
} ubo;

layout(push_constant) uniform Push {
    vec4 position; // ignore w
    vec4 color; // w is intesntiy
    float radius;
} push;

const float M_PI = 3.1415926538;
void main() {
    // Discard any pixels outside the radius
    float dis = sqrt(dot(fragOffset, fragOffset));
    if (dis >= 1.0) {
        discard;
    }
    
    // Temporary? looks cool
    float cosDis = 0.5 * (cos(dis * M_PI * 1.5) + 1.0);
    outColor = vec4(push.color.xyz * cosDis, cosDis);

    // Without the white center
    // float cosDis = 0.5 * (cos(dis * M_PI) + 1.0);
    // outColor = vec4(push.color.xyz, cosDis);

    // With the white center (default)
    // float cosDis = 0.5 * (cos(dis * M_PI) + 1.0);
    // outColor = vec4(push.color.xyz + cosDis, cosDis);
}