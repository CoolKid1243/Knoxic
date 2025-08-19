#pragma once

#include "../../graphics/vulkan/knoxic_vk_pipeline.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../graphics/knoxic_frame_info.hpp"

#include <memory>

namespace knoxic {

    class PointLightSystem {
        public:
            PointLightSystem(KnoxicDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
            ~PointLightSystem();

            PointLightSystem(const PointLightSystem &) = delete;
            PointLightSystem &operator=(const PointLightSystem &) = delete;

            void update(FrameInfo &frameInfo, GlobalUbo &ubo);
            void render(FrameInfo &frameInfo);

        private:
            void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
            void createPipeline(VkRenderPass renderPass);

            KnoxicDevice &knoxicDevice;
            std::unique_ptr<KnoxicPipeline> knoxicPipeline;
            VkPipelineLayout pipelineLayout;
    };
}