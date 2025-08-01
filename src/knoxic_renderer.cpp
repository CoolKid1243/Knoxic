#include "knoxic_renderer.hpp"
#include "knoxic_device.hpp"
#include "knoxic_swap_chain.hpp"

#include <array>
#include <cassert>
#include <memory>
#include <stdexcept>

namespace knoxic {

    KnoxicRenderer::KnoxicRenderer(KnoxicWindow &window, KnoxicDevice &device) : knoxicWindow{window}, knoxicDevice(device) {;
        recreateSwapChain();
        createCommandBuffers();
    }

    KnoxicRenderer::~KnoxicRenderer() { freeCommandBuffers(); }

    void KnoxicRenderer::recreateSwapChain() {
        auto extent = knoxicWindow.getExtent();
        while (extent.width == 0 || extent.height == 0) {
            extent = knoxicWindow.getExtent();
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(knoxicDevice.device());

        if (knoxicSwapChain == nullptr) {
            knoxicSwapChain = std::make_unique<KnoxicSwapChain>(knoxicDevice, extent);
        } else {
            std::shared_ptr<KnoxicSwapChain> oldSwapChain = std::move(knoxicSwapChain);
            knoxicSwapChain = std::make_unique<KnoxicSwapChain>(knoxicDevice, extent, oldSwapChain);

            if (!oldSwapChain->compareSwapFormats(*knoxicSwapChain.get())) {
                std::runtime_error("Swap chain image(or depth) format has changed!");
            }
        }
    }

    void KnoxicRenderer::createCommandBuffers() {
        commandBuffers.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool = knoxicDevice.getCommandPool();
        allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(knoxicDevice.device(), &allocateInfo, commandBuffers.data()) != 
        VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void KnoxicRenderer::freeCommandBuffers() {
        vkFreeCommandBuffers(
            knoxicDevice.device(), 
            knoxicDevice.getCommandPool(), 
            static_cast<uint32_t>(commandBuffers.size()), 
            commandBuffers.data()
        );
        commandBuffers.clear();
    }

    VkCommandBuffer KnoxicRenderer::beginFrame() {
        assert(!isFrameStarted && "Can't call beginFrame while allready in progress");

        auto result = knoxicSwapChain->acquireNextImage(&currentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return nullptr;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        isFrameStarted = true;

        auto commandBuffer = getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        return commandBuffer;
    }
    
    void KnoxicRenderer::endFrame() {
        assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
        auto commandBuffer = getCurrentCommandBuffer();

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        auto result = knoxicSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || knoxicWindow.wasWindowResized()) {
            knoxicWindow.resetWindowResizedFlag();
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        isFrameStarted = false;
        currentFrameIndex = (currentFrameIndex + 1) % KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT;
    }

    void KnoxicRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Can't call beginSwapChainRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = knoxicSwapChain->getRenderPass();
        renderPassInfo.framebuffer = knoxicSwapChain->getFrameBuffer(currentImageIndex);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = knoxicSwapChain->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f}; // window background color
        clearValues[1].depthStencil = {1.0f, 0};
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(knoxicSwapChain->getSwapChainExtent().width);
        viewport.height = static_cast<float>(knoxicSwapChain->getSwapChainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, knoxicSwapChain->getSwapChainExtent()};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void KnoxicRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
        assert(isFrameStarted && "Can't call endSwapChainRenderPass while frame is not in progress");
        assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

        vkCmdEndRenderPass(commandBuffer);
    }
}