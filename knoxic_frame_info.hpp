#pragma once

#include "knoxic_camera.hpp"

#include <vulkan/vulkan.h>

namespace knoxic {

    struct FrameInfo {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        KnoxicCamera &camera;
    };
}