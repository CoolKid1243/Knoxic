#pragma once

#include "../camera/knoxic_camera.hpp"

#include <vulkan/vulkan.h>

namespace knoxic {

    #define MAX_LIGHTS 1000

    struct PointLight {
        glm::vec4 position{}; // ignore w
        glm::vec4 color{}; // w is intensity
    };

    struct SpotLight {
        glm::vec4 position{}; // ignore w
        glm::vec4 direction{}; // ignore w
        glm::vec4 color{}; // w is intensity
        float innerCutoff{}; // cos of inner angle
        float outerCutoff{}; // cos of outer angle
        float padding1{};
        float padding2{};
    };

    struct DirectionalLight {
        glm::vec4 direction{}; // ignore w
        glm::vec4 color{}; // w is intensity
    };

    struct GlobalUbo {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::mat4 inverseView{1.0f};
        glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f}; // w is intensity
        PointLight pointLights[MAX_LIGHTS];
        int numLights;
        int numSpotLights;
        int numDirectionalLights;
        int padding;
        SpotLight spotLights[MAX_LIGHTS];
        DirectionalLight directionalLights[MAX_LIGHTS];
    };

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        KnoxicCamera &camera;
        VkDescriptorSet globalDescriptorSet;
    };
}