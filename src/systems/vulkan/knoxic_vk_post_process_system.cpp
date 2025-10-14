#include "knoxic_vk_post_process_system.hpp"
#include "../../core/vulkan/knoxic_vk_swap_chain.hpp"

#include <stdexcept>
#include <array>

namespace knoxic {

    PostProcessSystem::PostProcessSystem(KnoxicDevice& device, VkExtent2D extent) : knoxicDevice{device}, extent{extent} {
        createHDRResources();
        createBloomResources();
        createRenderPasses();
        createFramebuffers();
        createDescriptorSetLayouts();
        createDescriptorSets();
        createPipelines();
    }

    PostProcessSystem::~PostProcessSystem() {
        cleanup();
    }

    void PostProcessSystem::cleanup() {
        vkDeviceWaitIdle(knoxicDevice.device());

        // Destroy pipelines
        if (postProcessPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(knoxicDevice.device(), postProcessPipelineLayout, nullptr);
            postProcessPipelineLayout = VK_NULL_HANDLE;
        }
        if (blurPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(knoxicDevice.device(), blurPipelineLayout, nullptr);
            blurPipelineLayout = VK_NULL_HANDLE;
        }
        if (brightnessExtractPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(knoxicDevice.device(), brightnessExtractPipelineLayout, nullptr);
            brightnessExtractPipelineLayout = VK_NULL_HANDLE;
        }

        postProcessPipeline.reset();
        blurPipeline.reset();
        brightnessExtractPipeline.reset();

        // Destroy samplers
        if (hdrSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), hdrSampler, nullptr);
            hdrSampler = VK_NULL_HANDLE;
        }
        if (bloomSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), bloomSampler, nullptr);
            bloomSampler = VK_NULL_HANDLE;
        }

        // Destroy descriptor layouts and pool
        brightnessExtractSetLayout.reset();
        blurSetLayout.reset();
        postProcessSetLayout.reset();
        descriptorPool.reset();

        // Destroy bloom framebuffers and images
        for (int i = 0; i < 2; i++) {
            for (size_t j = 0; j < KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT; j++) {
                if (j < bloomFramebuffers[i].size()) {
                    vkDestroyFramebuffer(knoxicDevice.device(), bloomFramebuffers[i][j], nullptr);
                }
                if (j < bloomImageViews[i].size()) {
                    vkDestroyImageView(knoxicDevice.device(), bloomImageViews[i][j], nullptr);
                }
                if (j < bloomImages[i].size()) {
                    vkDestroyImage(knoxicDevice.device(), bloomImages[i][j], nullptr);
                }
                if (j < bloomImageMemories[i].size()) {
                    vkFreeMemory(knoxicDevice.device(), bloomImageMemories[i][j], nullptr);
                }
            }
            bloomFramebuffers[i].clear();
            bloomImageViews[i].clear();
            bloomImages[i].clear();
            bloomImageMemories[i].clear();
        }

        if (bloomRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(knoxicDevice.device(), bloomRenderPass, nullptr);
            bloomRenderPass = VK_NULL_HANDLE;
        }

        // Destroy HDR framebuffers and images
        for (size_t i = 0; i < hdrFramebuffers.size(); i++) {
            vkDestroyFramebuffer(knoxicDevice.device(), hdrFramebuffers[i], nullptr);
        }
        for (size_t i = 0; i < hdrDepthImageViews.size(); i++) {
            vkDestroyImageView(knoxicDevice.device(), hdrDepthImageViews[i], nullptr);
        }
        for (size_t i = 0; i < hdrDepthImages.size(); i++) {
            vkDestroyImage(knoxicDevice.device(), hdrDepthImages[i], nullptr);
        }
        for (size_t i = 0; i < hdrDepthImageMemories.size(); i++) {
            vkFreeMemory(knoxicDevice.device(), hdrDepthImageMemories[i], nullptr);
        }
        for (size_t i = 0; i < hdrImageViews.size(); i++) {
            vkDestroyImageView(knoxicDevice.device(), hdrImageViews[i], nullptr);
        }
        for (size_t i = 0; i < hdrImages.size(); i++) {
            vkDestroyImage(knoxicDevice.device(), hdrImages[i], nullptr);
        }
        for (size_t i = 0; i < hdrImageMemories.size(); i++) {
            vkFreeMemory(knoxicDevice.device(), hdrImageMemories[i], nullptr);
        }

        hdrFramebuffers.clear();
        hdrDepthImageViews.clear();
        hdrDepthImages.clear();
        hdrDepthImageMemories.clear();
        hdrImageViews.clear();
        hdrImages.clear();
        hdrImageMemories.clear();

        if (hdrRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(knoxicDevice.device(), hdrRenderPass, nullptr);
            hdrRenderPass = VK_NULL_HANDLE;
        }
    }

    void PostProcessSystem::recreate(VkExtent2D newExtent) {
        extent = newExtent;
        cleanup();
        createHDRResources();
        createBloomResources();
        createRenderPasses();
        createFramebuffers();
        createDescriptorSetLayouts();
        createDescriptorSets();
        createPipelines();
    }

    void PostProcessSystem::createHDRResources() {
        hdrImages.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        hdrImageMemories.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        hdrImageViews.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        hdrDepthImages.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        hdrDepthImageMemories.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        hdrDepthImageViews.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);

        VkFormat hdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        for (size_t i = 0; i < KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
            // Create HDR color image
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = extent.width;
            imageInfo.extent.height = extent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = hdrFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            knoxicDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                hdrImages[i], hdrImageMemories[i]);

            // Create image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = hdrImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = hdrFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(knoxicDevice.device(), &viewInfo, nullptr, &hdrImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create HDR image view!");
            }

            // Create depth image
            VkFormat depthFormat = knoxicDevice.findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );

            VkImageCreateInfo depthImageInfo{};
            depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
            depthImageInfo.extent.width = extent.width;
            depthImageInfo.extent.height = extent.height;
            depthImageInfo.extent.depth = 1;
            depthImageInfo.mipLevels = 1;
            depthImageInfo.arrayLayers = 1;
            depthImageInfo.format = depthFormat;
            depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            knoxicDevice.createImageWithInfo(depthImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                hdrDepthImages[i], hdrDepthImageMemories[i]);

            // Create depth image view
            VkImageViewCreateInfo depthViewInfo{};
            depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthViewInfo.image = hdrDepthImages[i];
            depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthViewInfo.format = depthFormat;
            depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthViewInfo.subresourceRange.baseMipLevel = 0;
            depthViewInfo.subresourceRange.levelCount = 1;
            depthViewInfo.subresourceRange.baseArrayLayer = 0;
            depthViewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(knoxicDevice.device(), &depthViewInfo, nullptr, &hdrDepthImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create depth image view!");
            }
        }

        // Create HDR sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(knoxicDevice.device(), &samplerInfo, nullptr, &hdrSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create HDR sampler!");
        }
    }

    void PostProcessSystem::createBloomResources() {
        for (int i = 0; i < 2; i++) {
            bloomImages[i].resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
            bloomImageMemories[i].resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
            bloomImageViews[i].resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        }

        VkFormat bloomFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

        for (int pingPong = 0; pingPong < 2; pingPong++) {
            for (size_t i = 0; i < KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = extent.width / 2;
                imageInfo.extent.height = extent.height / 2;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = bloomFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                knoxicDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    bloomImages[pingPong][i], bloomImageMemories[pingPong][i]);

                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = bloomImages[pingPong][i];
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = bloomFormat;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(knoxicDevice.device(), &viewInfo, nullptr, &bloomImageViews[pingPong][i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create bloom image view!");
                }
            }
        }

        // Create bloom sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(knoxicDevice.device(), &samplerInfo, nullptr, &bloomSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create bloom sampler!");
        }
    }

    void PostProcessSystem::createRenderPasses() {
        // HDR render pass
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = knoxicDevice.findSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(knoxicDevice.device(), &renderPassInfo, nullptr, &hdrRenderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create HDR render pass!");
            }
        }

        // Bloom render pass (simple color attachment for bloom buffers)
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(knoxicDevice.device(), &renderPassInfo, nullptr, &bloomRenderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create bloom render pass!");
            }
        }
    }

    void PostProcessSystem::createFramebuffers() {
        // HDR framebuffers
        hdrFramebuffers.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < hdrFramebuffers.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                hdrImageViews[i],
                hdrDepthImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = hdrRenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = extent.width;
            framebufferInfo.height = extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(knoxicDevice.device(), &framebufferInfo, nullptr, &hdrFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create HDR framebuffer!");
            }
        }

        // Bloom framebuffers
        for (int pingPong = 0; pingPong < 2; pingPong++) {
            bloomFramebuffers[pingPong].resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (size_t i = 0; i < bloomFramebuffers[pingPong].size(); i++) {
                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = bloomRenderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = &bloomImageViews[pingPong][i];
                framebufferInfo.width = extent.width / 2;
                framebufferInfo.height = extent.height / 2;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(knoxicDevice.device(), &framebufferInfo, nullptr, &bloomFramebuffers[pingPong][i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create bloom framebuffer!");
                }
            }
        }
    }

    void PostProcessSystem::createDescriptorSetLayouts() {
        // Brightness extract: single texture input
        brightnessExtractSetLayout = KnoxicDescriptorSetLayout::Builder(knoxicDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        // Blur: single texture input
        blurSetLayout = KnoxicDescriptorSetLayout::Builder(knoxicDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();

        // Post process: scene texture + bloom texture
        postProcessSetLayout = KnoxicDescriptorSetLayout::Builder(knoxicDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    }

    void PostProcessSystem::createDescriptorSets() {
        // Create descriptor pool
        descriptorPool = KnoxicDescriptorPool::Builder(knoxicDevice)
            .setMaxSets(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT * 6)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT * 8)
            .build();

        // Brightness extract descriptor sets
        brightnessExtractDescriptorSets.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < brightnessExtractDescriptorSets.size(); i++) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = hdrImageViews[i];
            imageInfo.sampler = hdrSampler;

            KnoxicDescriptorWriter(*brightnessExtractSetLayout, *descriptorPool)
                .writeImage(0, &imageInfo)
                .build(brightnessExtractDescriptorSets[i]);
        }

        // Blur descriptor sets
        for (int pingPong = 0; pingPong < 2; pingPong++) {
            blurDescriptorSets[pingPong].resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
            for (size_t i = 0; i < blurDescriptorSets[pingPong].size(); i++) {
                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = bloomImageViews[pingPong][i];
                imageInfo.sampler = bloomSampler;

                KnoxicDescriptorWriter(*blurSetLayout, *descriptorPool)
                    .writeImage(0, &imageInfo)
                    .build(blurDescriptorSets[pingPong][i]);
            }
        }

        // Post process descriptor sets
        postProcessDescriptorSets.resize(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < postProcessDescriptorSets.size(); i++) {
            VkDescriptorImageInfo sceneImageInfo{};
            sceneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            sceneImageInfo.imageView = hdrImageViews[i];
            sceneImageInfo.sampler = hdrSampler;

            VkDescriptorImageInfo bloomImageInfo{};
            bloomImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            bloomImageInfo.imageView = bloomImageViews[0][i]; // Use buffer 0
            bloomImageInfo.sampler = bloomSampler;

            KnoxicDescriptorWriter(*postProcessSetLayout, *descriptorPool)
                .writeImage(0, &sceneImageInfo)
                .writeImage(1, &bloomImageInfo)
                .build(postProcessDescriptorSets[i]);
        }
    }

    void PostProcessSystem::createPipelines() {
        // Brightness extract pipeline
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(float);

            VkDescriptorSetLayout setLayout = brightnessExtractSetLayout->getDescriptorSetLayout();
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &setLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

            if (vkCreatePipelineLayout(knoxicDevice.device(), &pipelineLayoutInfo, nullptr, &brightnessExtractPipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create brightness extract pipeline layout!");
            }

            PipelineConfigInfo pipelineConfig{};
            KnoxicPipeline::defaultPipelineConfigInfo(pipelineConfig);
            pipelineConfig.renderPass = bloomRenderPass;
            pipelineConfig.pipelineLayout = brightnessExtractPipelineLayout;
            pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
            pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

            brightnessExtractPipeline = std::make_unique<KnoxicPipeline>(
                knoxicDevice,
                "shaders/vk_brightness_extract.vert.spv",
                "shaders/vk_brightness_extract.frag.spv",
                pipelineConfig
            );
        }

        // Blur pipeline
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(glm::vec2);

            VkDescriptorSetLayout setLayout = blurSetLayout->getDescriptorSetLayout();
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &setLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

            if (vkCreatePipelineLayout(knoxicDevice.device(), &pipelineLayoutInfo, nullptr, &blurPipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create blur pipeline layout!");
            }

            PipelineConfigInfo pipelineConfig{};
            KnoxicPipeline::defaultPipelineConfigInfo(pipelineConfig);
            pipelineConfig.renderPass = bloomRenderPass;
            pipelineConfig.pipelineLayout = blurPipelineLayout;
            pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
            pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

            blurPipeline = std::make_unique<KnoxicPipeline>(
                knoxicDevice,
                "shaders/vk_blur.vert.spv",
                "shaders/vk_blur.frag.spv",
                pipelineConfig
            );
        }

        // Post process pipeline layout
        {
            struct PostProcessPushConstants {
                float bloomIntensity;
                float exposure;
                float gamma;
                int bloomEnabled;
            };

            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pushConstantRange.offset = 0;
            pushConstantRange.size = sizeof(PostProcessPushConstants);

            VkDescriptorSetLayout setLayout = postProcessSetLayout->getDescriptorSetLayout();
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &setLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

            if (vkCreatePipelineLayout(knoxicDevice.device(), &pipelineLayoutInfo, nullptr, &postProcessPipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create post process pipeline layout!");
            }

            postProcessPipeline = nullptr; // Will be created on first use with correct render pass
        }
    }

    void PostProcessSystem::renderPostProcess(
        VkCommandBuffer commandBuffer,
        int frameIndex,
        const PostProcessingComponent& settings
    ) {
        // Step 1: Extract bright pixels
        if (settings.bloomEnabled) {
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = bloomRenderPass;
            renderPassInfo.framebuffer = bloomFramebuffers[0][frameIndex];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {extent.width / 2, extent.height / 2};

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(extent.width / 2);
            viewport.height = static_cast<float>(extent.height / 2);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            VkRect2D scissor{{0, 0}, {extent.width / 2, extent.height / 2}};
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            brightnessExtractPipeline->bind(commandBuffer);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                brightnessExtractPipelineLayout, 0, 1, &brightnessExtractDescriptorSets[frameIndex], 0, nullptr);
            
            float threshold = settings.bloomThreshold;
            vkCmdPushConstants(commandBuffer, brightnessExtractPipelineLayout, 
                VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &threshold);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Full-screen triangle
            vkCmdEndRenderPass(commandBuffer);

            // Step 2: Blur passes
            for (int i = 0; i < settings.bloomIterations; i++) {
                // Horizontal blur
                {
                    renderPassInfo.framebuffer = bloomFramebuffers[1][frameIndex];
                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                    blurPipeline->bind(commandBuffer);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        blurPipelineLayout, 0, 1, &blurDescriptorSets[0][frameIndex], 0, nullptr);

                    glm::vec2 direction(1.0f, 0.0f);
                    vkCmdPushConstants(commandBuffer, blurPipelineLayout,
                        VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2), &direction);

                    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
                    vkCmdEndRenderPass(commandBuffer);
                }

                // Vertical blur
                {
                    renderPassInfo.framebuffer = bloomFramebuffers[0][frameIndex];
                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                    blurPipeline->bind(commandBuffer);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        blurPipelineLayout, 0, 1, &blurDescriptorSets[1][frameIndex], 0, nullptr);

                    glm::vec2 direction(0.0f, 1.0f);
                    vkCmdPushConstants(commandBuffer, blurPipelineLayout,
                        VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2), &direction);

                    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
                    vkCmdEndRenderPass(commandBuffer);
                }
            }
        }
        
    }

    void PostProcessSystem::renderFinalComposite(
        VkCommandBuffer commandBuffer,
        VkRenderPass swapchainRenderPass,
        int frameIndex,
        const PostProcessingComponent& settings
    ) {
        // Create post-process pipeline if needed
        if (!postProcessPipeline) {
            PipelineConfigInfo pipelineConfig{};
            KnoxicPipeline::defaultPipelineConfigInfo(pipelineConfig);
            pipelineConfig.renderPass = swapchainRenderPass;
            pipelineConfig.pipelineLayout = postProcessPipelineLayout;
            pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
            pipelineConfig.depthStencilInfo.depthWriteEnable = VK_FALSE;

            postProcessPipeline = std::make_unique<KnoxicPipeline>(
                knoxicDevice,
                "shaders/vk_post_process.vert.spv",
                "shaders/vk_post_process.frag.spv",
                pipelineConfig
            );
        }

        // Set viewport and scissor for full-screen render
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0, 0}, extent};
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind pipeline and render full-screen quad
        postProcessPipeline->bind(commandBuffer);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            postProcessPipelineLayout, 0, 1, &postProcessDescriptorSets[frameIndex], 0, nullptr);

        struct PostProcessPushConstants {
            float bloomIntensity;
            float exposure;
            float gamma;
            int bloomEnabled;
        };

        PostProcessPushConstants pushConstants{};
        pushConstants.bloomIntensity = settings.bloomIntensity;
        pushConstants.exposure = settings.exposure;
        pushConstants.gamma = settings.gamma;
        pushConstants.bloomEnabled = settings.bloomEnabled ? 1 : 0;

        vkCmdPushConstants(commandBuffer, postProcessPipelineLayout,
            VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PostProcessPushConstants), &pushConstants);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0); // Full-screen triangle
    }
}

