#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D hdrTexture;

layout(push_constant) uniform Push {
    float threshold;
} push;

void main() {
    vec3 color = texture(hdrTexture, fragTexCoord).rgb;
    
    // Calculate luminance
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    
    // Extract bright pixels above threshold
    if (brightness > push.threshold) {
        outColor = vec4(color, 1.0);
    } else {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}