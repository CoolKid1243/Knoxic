#pragma once

#include "../../graphics/vulkan/knoxic_vk_pipeline.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../graphics/knoxic_frame_info.hpp"
#include "../../core/ecs/ecs_systems.hpp"

#include <memory>

namespace knoxic {

    class SpotLightSystem {
    public:
        SpotLightSystem(KnoxicDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout,
                        std::shared_ptr<SpotLightECSSystem> ecsSpotLightSystem);
        ~SpotLightSystem();

        SpotLightSystem(const SpotLightSystem &) = delete;
        SpotLightSystem &operator=(const SpotLightSystem &) = delete;

        void update(FrameInfo &frameInfo, GlobalUbo &ubo);
        void render(FrameInfo &frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        KnoxicDevice &knoxicDevice;
        std::unique_ptr<KnoxicPipeline> knoxicPipeline;
        VkPipelineLayout pipelineLayout;

        // ECS
        std::shared_ptr<SpotLightECSSystem> spotLightSystem;
    };
}