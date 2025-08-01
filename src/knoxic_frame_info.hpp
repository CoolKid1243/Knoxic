#pragma once

#include "knoxic_camera.hpp"
#include "knoxic_game_object.hpp"

#include <vulkan/vulkan.h>

namespace knoxic {

    #define MAX_LIGHTS 10

    struct PointLight {
        glm::vec4 position{}; // ignore w
        glm::vec4 color{}; // w is intensity
    };

    struct GlobalUbo {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
        glm::vec4 ambientLightColor{1.0f, 1.0f, 1.0f, 0.02f}; // w is intensity
        PointLight pointLights[MAX_LIGHTS];
        int numLights;
    };

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        KnoxicCamera &camera;
        VkDescriptorSet globalDescriptorSet;
        KnoxicGameObject::Map &gameObjects;
    };
}