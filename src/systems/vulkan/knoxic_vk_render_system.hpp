#pragma once

#include "../../graphics/vulkan/knoxic_vk_pipeline.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../graphics/knoxic_frame_info.hpp"
#include "../../core/ecs/ecs_systems.hpp"

#include <memory>

namespace knoxic {

    class RenderSystem {
    public:
        RenderSystem(KnoxicDevice &device, VkRenderPass renderPass, 
            VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout materialSetLayout,
            std::shared_ptr<RenderableSystem> ecsRenderableSystem);
        ~RenderSystem();

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem &operator=(const RenderSystem &) = delete;

        void renderGameObjects(FrameInfo &frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout, VkDescriptorSetLayout materialSetLayout);
        void createPipeline(VkRenderPass renderPass);

        KnoxicDevice &knoxicDevice;
        std::unique_ptr<KnoxicPipeline> knoxicPipeline;
        VkPipelineLayout pipelineLayout;

        // ECS
        std::shared_ptr<RenderableSystem> renderableSystem;
    };
}