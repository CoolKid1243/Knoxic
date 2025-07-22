#pragma once

#include "knoxic_pipeline.hpp"
#include "knoxic_window.hpp"
#include "knoxic_device.hpp"
#include "knoxic_swap_chain.hpp"
#include "knoxic_model.hpp"

#include <memory>
#include <vector>

namespace knoxic {

    class App {
        public:
            static constexpr int WIDTH = 1152;
            static constexpr int HEIGHT = 758;

            App();
            ~App();

            App(const App &) = delete;
            App &operator=(const App &) = delete;

            void run();

        private:
            void loadModels();
            void createPipelineLayout();
            void createPipeline();
            void createCommandBuffers();
            void freeCommandBuffers();
            void drawFrame();
            void recreateSwapChain();
            void recordCommandBuffer(int imageIndex);

            KnoxicWindow knoxicWindow{WIDTH, HEIGHT, "Knoxic"};
            KnoxicDevice knoxicDevice{knoxicWindow};
            std::unique_ptr<KnoxicSwapChain> knoxicSwapChain;
            std::unique_ptr<KnoxicPipeline> knoxicPipeline;
            VkPipelineLayout pipelineLayout;
            std::vector<VkCommandBuffer> commandBuffers;
            std::unique_ptr<KnoxicModel> knoxicModel;
    };
}