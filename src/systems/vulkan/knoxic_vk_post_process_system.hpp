#pragma once

#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../core/vulkan/knoxic_vk_descriptors.hpp"
#include "../../graphics/vulkan/knoxic_vk_pipeline.hpp"
#include "../../core/ecs/components.hpp"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace knoxic {

    class PostProcessSystem {
    public:
        PostProcessSystem(KnoxicDevice& device, VkExtent2D extent);
        ~PostProcessSystem();

        PostProcessSystem(const PostProcessSystem&) = delete;
        PostProcessSystem& operator=(const PostProcessSystem&) = delete;

        // Get the HDR render pass for rendering the scene
        VkRenderPass getHDRRenderPass() const { return hdrRenderPass; }
        VkFramebuffer getHDRFramebuffer(int frameIndex) const { return hdrFramebuffers[frameIndex]; }
        
        // Apply post-processing effects
        void renderPostProcess(
            VkCommandBuffer commandBuffer,
            int frameIndex,
            const PostProcessingComponent& settings
        );

        // Render final composite to current render pass
        void renderFinalComposite(
            VkCommandBuffer commandBuffer,
            VkRenderPass swapchainRenderPass,
            int frameIndex,
            const PostProcessingComponent& settings
        );

        // Recreate resources when window is resized
        void recreate(VkExtent2D newExtent);

    private:
        void createHDRResources();
        void createBloomResources();
        void createRenderPasses();
        void createFramebuffers();
        void createDescriptorSetLayouts();
        void createDescriptorSets();
        void createPipelines();
        
        void cleanup();

        KnoxicDevice& knoxicDevice;
        VkExtent2D extent;

        // HDR scene render target
        VkRenderPass hdrRenderPass;
        std::vector<VkImage> hdrImages;
        std::vector<VkDeviceMemory> hdrImageMemories;
        std::vector<VkImageView> hdrImageViews;
        std::vector<VkFramebuffer> hdrFramebuffers;
        std::vector<VkImage> hdrDepthImages;
        std::vector<VkDeviceMemory> hdrDepthImageMemories;
        std::vector<VkImageView> hdrDepthImageViews;

        // Bloom ping-pong buffers
        VkRenderPass bloomRenderPass;
        std::vector<VkImage> bloomImages[2];
        std::vector<VkDeviceMemory> bloomImageMemories[2];
        std::vector<VkImageView> bloomImageViews[2];
        std::vector<VkFramebuffer> bloomFramebuffers[2];

        // Descriptor pools and layouts
        std::unique_ptr<KnoxicDescriptorPool> descriptorPool;
        std::unique_ptr<KnoxicDescriptorSetLayout> brightnessExtractSetLayout;
        std::unique_ptr<KnoxicDescriptorSetLayout> blurSetLayout;
        std::unique_ptr<KnoxicDescriptorSetLayout> postProcessSetLayout;

        // Descriptor sets
        std::vector<VkDescriptorSet> brightnessExtractDescriptorSets;
        std::vector<VkDescriptorSet> blurDescriptorSets[2];
        std::vector<VkDescriptorSet> postProcessDescriptorSets;

        // Pipelines
        std::unique_ptr<KnoxicPipeline> brightnessExtractPipeline;
        VkPipelineLayout brightnessExtractPipelineLayout;
        
        std::unique_ptr<KnoxicPipeline> blurPipeline;
        VkPipelineLayout blurPipelineLayout;
        
        std::unique_ptr<KnoxicPipeline> postProcessPipeline;
        VkPipelineLayout postProcessPipelineLayout;

        // Samplers
        VkSampler hdrSampler;
        VkSampler bloomSampler;
    };
}