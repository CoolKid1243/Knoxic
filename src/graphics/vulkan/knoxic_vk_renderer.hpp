#pragma once

#include "../../core/knoxic_window.hpp"
#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../core/vulkan/knoxic_vk_swap_chain.hpp"

#include <memory>
#include <vector>
#include <cassert>

namespace knoxic {

    class KnoxicRenderer {
        public:
            KnoxicRenderer(KnoxicWindow &window, KnoxicDevice &device);
            ~KnoxicRenderer();

            KnoxicRenderer(const KnoxicRenderer &) = delete;
            KnoxicRenderer &operator=(const KnoxicRenderer &) = delete;

            VkRenderPass getSwapChainRenderPass() const { return knoxicSwapChain->getRenderPass(); }
            float getAspectRatio() const { return knoxicSwapChain->extentAspectRatio(); }
            bool isFrameInProgress() const { return isFrameStarted; }

            VkCommandBuffer getCurrentCommandBuffer() const { 
                assert(isFrameStarted && "Cannot get command buffer when frame is not in progress");
                return commandBuffers[currentFrameIndex]; 
            }

            int getFrameIndex() const {
                assert(isFrameStarted && "Cannot get frame index when frame is not in progress");
                return currentFrameIndex;
            }

            VkCommandBuffer beginFrame();
            void endFrame();
            void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
            void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

        private:
            void createCommandBuffers();
            void freeCommandBuffers();
            void recreateSwapChain();

            KnoxicWindow &knoxicWindow;
            KnoxicDevice &knoxicDevice;
            std::unique_ptr<KnoxicSwapChain> knoxicSwapChain;
            std::vector<VkCommandBuffer> commandBuffers;

            uint32_t currentImageIndex;
            int currentFrameIndex{0};
            bool isFrameStarted{false};
    };
}