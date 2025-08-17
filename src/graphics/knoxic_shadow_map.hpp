#pragma once

#include "../core/knoxic_device.hpp"

#include <vulkan/vulkan.h>
#include <memory>

namespace knoxic {

    class KnoxicShadowMap {
        public:
            static constexpr int SHADOW_MAP_SIZE = 2048;
            
            KnoxicShadowMap(KnoxicDevice &device, uint32_t shadowMapCount = 1);
            ~KnoxicShadowMap();

            KnoxicShadowMap(const KnoxicShadowMap &) = delete;
            KnoxicShadowMap &operator=(const KnoxicShadowMap &) = delete;

            VkRenderPass getShadowRenderPass() const { return shadowRenderPass; }
            VkFramebuffer getShadowFramebuffer(uint32_t index) const { return shadowFramebuffers[index]; }
            VkImageView getShadowImageView(uint32_t index) const { return shadowImageViews[index]; }
            VkSampler getShadowSampler() const { return shadowSampler; }
            VkExtent2D getShadowExtent() const { return {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE}; }
            uint32_t getShadowMapCount() const { return shadowMapCount; }

        private:
            void createShadowImages();
            void createShadowRenderPass();
            void createShadowFramebuffers();
            void createShadowSampler();

            KnoxicDevice &knoxicDevice;
            uint32_t shadowMapCount;

            VkRenderPass shadowRenderPass;
            std::vector<VkImage> shadowImages;
            std::vector<VkDeviceMemory> shadowImageMemories;
            std::vector<VkImageView> shadowImageViews;
            std::vector<VkFramebuffer> shadowFramebuffers;
            VkSampler shadowSampler;
    };
}
