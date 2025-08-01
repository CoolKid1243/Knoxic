#pragma once

#include "knoxic_camera.hpp"
#include "knoxic_game_object.hpp"

#include <vulkan/vulkan.h>

namespace knoxic {

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        KnoxicCamera &camera;
        VkDescriptorSet globalDescriptorSet;
        KnoxicGameObject::Map &gameObjects;
    };
}