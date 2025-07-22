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
            void drawFrame();

            KnoxicWindow knoxicWindow{WIDTH, HEIGHT, "Knoxic"};
            KnoxicDevice knoxicDevice{knoxicWindow};
            KnoxicSwapChain knoxicSwapChain{knoxicDevice, knoxicWindow.getExtent()};
            std::unique_ptr<KnoxicPipeline> knoxicPipeline;
            VkPipelineLayout pipelineLayout;
            std::vector<VkCommandBuffer> commandBuffers;
            std::unique_ptr<KnoxicModel> knoxicModel;
    };
}