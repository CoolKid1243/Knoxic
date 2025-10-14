#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

layout(push_constant) uniform Push {
    vec2 direction;
} push;

// Gaussian blur weights
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / textureSize(inputTexture, 0);
    vec3 result = texture(inputTexture, fragTexCoord).rgb * weights[0];
    
    for(int i = 1; i < 5; i++) {
        vec2 offset = push.direction * texelSize * float(i);
        result += texture(inputTexture, fragTexCoord + offset).rgb * weights[i];
        result += texture(inputTexture, fragTexCoord - offset).rgb * weights[i];
    }
    
    outColor = vec4(result, 1.0);
}