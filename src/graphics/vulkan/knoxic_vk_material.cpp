#include "knoxic_vk_material.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <cstring>
#include <iostream>

#define ENGINE_DIR "../"

namespace knoxic {

    KnoxicMaterial::KnoxicMaterial(KnoxicDevice& device) : knoxicDevice{device} {
        createDefaultTexture();
    }

    KnoxicMaterial::~KnoxicMaterial() {
        // Clean up albedo texture
        if (albedoTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), albedoTextureSampler, nullptr);
        }
        if (albedoTextureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(knoxicDevice.device(), albedoTextureImageView, nullptr);
        }
        if (albedoTextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(knoxicDevice.device(), albedoTextureImage, nullptr);
            vkFreeMemory(knoxicDevice.device(), albedoTextureImageMemory, nullptr);
        }

        // Clean up normal texture
        if (normalTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), normalTextureSampler, nullptr);
        }
        if (normalTextureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(knoxicDevice.device(), normalTextureImageView, nullptr);
        }
        if (normalTextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(knoxicDevice.device(), normalTextureImage, nullptr);
            vkFreeMemory(knoxicDevice.device(), normalTextureImageMemory, nullptr);
        }

        // Clean up roughness texture
        if (roughnessTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), roughnessTextureSampler, nullptr);
        }
        if (roughnessTextureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(knoxicDevice.device(), roughnessTextureImageView, nullptr);
        }
        if (roughnessTextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(knoxicDevice.device(), roughnessTextureImage, nullptr);
            vkFreeMemory(knoxicDevice.device(), roughnessTextureImageMemory, nullptr);
        }

        // Clean up metallic texture
        if (metallicTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), metallicTextureSampler, nullptr);
        }
        if (metallicTextureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(knoxicDevice.device(), metallicTextureImageView, nullptr);
        }
        if (metallicTextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(knoxicDevice.device(), metallicTextureImage, nullptr);
            vkFreeMemory(knoxicDevice.device(), metallicTextureImageMemory, nullptr);
        }

        // Clean up default texture
        if (defaultTextureSampler != VK_NULL_HANDLE) {
            vkDestroySampler(knoxicDevice.device(), defaultTextureSampler, nullptr);
        }
        if (defaultTextureImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(knoxicDevice.device(), defaultTextureImageView, nullptr);
        }
        if (defaultTextureImage != VK_NULL_HANDLE) {
            vkDestroyImage(knoxicDevice.device(), defaultTextureImage, nullptr);
            vkFreeMemory(knoxicDevice.device(), defaultTextureImageMemory, nullptr);
        }
    }

    void KnoxicMaterial::createDefaultTexture() {
        // Create a simple 1x1 white texture as default
        const uint32_t texWidth = 1;
        const uint32_t texHeight = 1;
        const uint32_t texChannels = 4;
        unsigned char pixels[4] = {255, 255, 255, 255}; // White pixel

        VkDeviceSize imageSize = texWidth * texHeight * texChannels;

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        knoxicDevice.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(knoxicDevice.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(knoxicDevice.device(), stagingBufferMemory);

        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = texWidth;
        imageInfo.extent.height = texHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        knoxicDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            defaultTextureImage, defaultTextureImageMemory);

        // Transition image layout and copy buffer to image
        knoxicDevice.transitionImageLayout(defaultTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        knoxicDevice.copyBufferToImage(stagingBuffer, defaultTextureImage, texWidth, texHeight);
        knoxicDevice.transitionImageLayout(defaultTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Clean up staging buffer
        vkDestroyBuffer(knoxicDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(knoxicDevice.device(), stagingBufferMemory, nullptr);

        // Create image view
        createTextureImageView(defaultTextureImage, defaultTextureImageView);
        
        // Create sampler
        createTextureSampler(defaultTextureSampler);
    }

    void KnoxicMaterial::loadAlbedoTexture(const std::string& filepath) {
        try {
            createTextureImage(filepath, albedoTextureImage, albedoTextureImageMemory);
            createTextureImageView(albedoTextureImage, albedoTextureImageView);
            createTextureSampler(albedoTextureSampler);
            hasAlbedoTexture = true;
            // std::cout << filepath << std::endl; // print file path
        } catch (const std::exception& e) {
            std::cerr << "Failed to load albedo texture: " << e.what() << std::endl;
            hasAlbedoTexture = false;

            // Make the object neon pink if it cant load or find the material
            failedAlbedo = true;
            properties.albedo = glm::vec3(1.0f, 0.0f, 1.0f); // pink
            properties.emissionColor = glm::vec3(1.0f, 0.0f, 1.0f); // pink
            properties.emissionStrength = 1.0f;
        }
    }

    void KnoxicMaterial::loadNormalTexture(const std::string& filepath) {
        try {
            createTextureImage(filepath, normalTextureImage, normalTextureImageMemory);
            createTextureImageView(normalTextureImage, normalTextureImageView);
            createTextureSampler(normalTextureSampler);
            hasNormalTexture = true;
            // std::cout << filepath << std::endl; // print file path
        } catch (const std::exception& e) {
            std::cerr << "Failed to load normal texture: " << e.what() << std::endl;
            hasNormalTexture = false;
        }
    }

    void KnoxicMaterial::loadRoughnessMap(const std::string& filepath) {
        try {
            createTextureImage(filepath, roughnessTextureImage, roughnessTextureImageMemory);
            createTextureImageView(roughnessTextureImage, roughnessTextureImageView);
            createTextureSampler(roughnessTextureSampler);
            hasRoughnessTexture = true;
            // std::cout << filepath << std::endl; // print file path
        } catch (const std::exception& e) {
            std::cerr << "Failed to load roughness texture: " << e.what() << std::endl;
            hasRoughnessTexture = false;
        }
    }

    void KnoxicMaterial::loadMetallicMap(const std::string& filepath) {
        try {
            createTextureImage(filepath, metallicTextureImage, metallicTextureImageMemory);
            createTextureImageView(metallicTextureImage, metallicTextureImageView);
            createTextureSampler(metallicTextureSampler);
            hasMetallicTexture = true;
            // std::cout << filepath << std::endl; // print file path
        } catch (const std::exception& e) {
            std::cerr << "Failed to load metallic texture: " << e.what() << std::endl;
            hasMetallicTexture = false;
        }
    }

    void KnoxicMaterial::createTextureImage(const std::string& filepath, VkImage& image, VkDeviceMemory& imageMemory) {
        std::string fullPath = ENGINE_DIR + filepath;
        
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("Failed to load texture image: " + fullPath + " - " + stbi_failure_reason());
        }

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        knoxicDevice.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(knoxicDevice.device(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(knoxicDevice.device(), stagingBufferMemory);

        stbi_image_free(pixels);

        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(texWidth);
        imageInfo.extent.height = static_cast<uint32_t>(texHeight);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        knoxicDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

        // Transition image layout and copy buffer to image
        knoxicDevice.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        knoxicDevice.copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        knoxicDevice.transitionImageLayout(image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Clean up staging buffer
        vkDestroyBuffer(knoxicDevice.device(), stagingBuffer, nullptr);
        vkFreeMemory(knoxicDevice.device(), stagingBufferMemory, nullptr);
    }

    void KnoxicMaterial::createTextureImageView(VkImage image, VkImageView& imageView) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(knoxicDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture image view!");
        }
    }

    void KnoxicMaterial::createTextureSampler(VkSampler& sampler) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(knoxicDevice.getPhysicalDevice(), &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(knoxicDevice.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    void KnoxicMaterial::updateDescriptorSet(KnoxicDescriptorSetLayout& setLayout, KnoxicDescriptorPool& pool) {
        // Create descriptor set if not already created
        if (descriptorSet == VK_NULL_HANDLE) {
            if (!pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), descriptorSet)) {
                throw std::runtime_error("Failed to allocate material descriptor set!");
            }
        }

        KnoxicDescriptorWriter writer(setLayout, pool);
        
        VkDescriptorImageInfo albedoImageInfo{};
        albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        albedoImageInfo.imageView = hasAlbedoTexture ? albedoTextureImageView : defaultTextureImageView;
        albedoImageInfo.sampler = hasAlbedoTexture ? albedoTextureSampler : defaultTextureSampler;
        writer.writeImage(0, &albedoImageInfo);

        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = hasNormalTexture ? normalTextureImageView : defaultTextureImageView;
        normalImageInfo.sampler = hasNormalTexture ? normalTextureSampler : defaultTextureSampler;
        writer.writeImage(1, &normalImageInfo);

        VkDescriptorImageInfo roughnessImageInfo{};
        roughnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        roughnessImageInfo.imageView = hasRoughnessTexture ? roughnessTextureImageView : defaultTextureImageView;
        roughnessImageInfo.sampler = hasRoughnessTexture ? roughnessTextureSampler : defaultTextureSampler;
        writer.writeImage(2, &roughnessImageInfo);

        VkDescriptorImageInfo metallicImageInfo{};
        metallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        metallicImageInfo.imageView = hasMetallicTexture ? metallicTextureImageView : defaultTextureImageView;
        metallicImageInfo.sampler = hasMetallicTexture ? metallicTextureSampler : defaultTextureSampler;
        writer.writeImage(3, &metallicImageInfo);

        writer.overwrite(descriptorSet);
    }
}