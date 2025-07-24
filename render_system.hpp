#pragma once

#include "knoxic_pipeline.hpp"
#include "knoxic_device.hpp"
#include "knoxic_game_object.hpp"
#include "knoxic_camera.hpp"

#include <memory>
#include <vector>

namespace knoxic {

    class RenderSystem {
        public:
            RenderSystem(KnoxicDevice &device, VkRenderPass renderPass);
            ~RenderSystem();

            RenderSystem(const RenderSystem &) = delete;
            RenderSystem &operator=(const RenderSystem &) = delete;

            void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<KnoxicGameObject> &gameObjects, const KnoxicCamera &camera);

        private:
            void createPipelineLayout();
            void createPipeline(VkRenderPass renderPass);

            KnoxicDevice &knoxicDevice;
            std::unique_ptr<KnoxicPipeline> knoxicPipeline;
            VkPipelineLayout pipelineLayout;
    };
}