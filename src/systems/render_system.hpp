#pragma once

#include "../knoxic_pipeline.hpp"
#include "../knoxic_device.hpp"
#include "../knoxic_frame_info.hpp"

#include <memory>

namespace knoxic {

    class RenderSystem {
        public:
            RenderSystem(KnoxicDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
            ~RenderSystem();

            RenderSystem(const RenderSystem &) = delete;
            RenderSystem &operator=(const RenderSystem &) = delete;

            void renderGameObjects(FrameInfo &frameInfo);

        private:
            void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
            void createPipeline(VkRenderPass renderPass);

            KnoxicDevice &knoxicDevice;
            std::unique_ptr<KnoxicPipeline> knoxicPipeline;
            VkPipelineLayout pipelineLayout;
    };
}