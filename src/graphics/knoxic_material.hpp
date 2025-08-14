#pragma once

#include "../core/knoxic_device.hpp"
#include "../core/knoxic_descriptors.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vulkan/vulkan.h>

namespace knoxic {

    struct MaterialProperties {
        glm::vec3 albedo{1.0f, 1.0f, 1.0f};
        float metallic{0.0f};
        float roughness{0.5f};
        float ao{1.0f};
        glm::vec2 textureOffset{0.0f, 0.0f};
        glm::vec2 textureScale{1.0f, 1.0f};
    };

    class KnoxicMaterial {
        public:
            KnoxicMaterial(KnoxicDevice& device);
            ~KnoxicMaterial();

            KnoxicMaterial(const KnoxicMaterial&) = delete;
            KnoxicMaterial& operator=(const KnoxicMaterial&) = delete;

            void setAlbedo(const glm::vec3& color) { properties.albedo = color; }
            void setMetallic(float metallic) { properties.metallic = metallic; }
            void setRoughness(float roughness) { properties.roughness = roughness; }
            void setAO(float ao) { properties.ao = ao; }
            void setTextureOffset(const glm::vec2& offset) { properties.textureOffset = offset; }
            void setTextureScale(const glm::vec2& scale) { properties.textureScale = scale; }

            // Texture loading
            void loadTexture(const std::string& filepath);
            void loadNormalMap(const std::string& filepath);
            void loadRoughnessMap(const std::string& filepath);
            void loadMetallicMap(const std::string& filepath);

            // Getters
            const MaterialProperties& getProperties() const { return properties; }
            VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
            bool hasTextures() const { return hasAlbedoTexture || hasNormalTexture || hasRoughnessTexture || hasMetallicTexture; }

            // Update descriptor set
            void updateDescriptorSet(KnoxicDescriptorSetLayout& setLayout, KnoxicDescriptorPool& pool);

        private:
            void createDefaultTexture();
            void createTextureImage(const std::string& filepath, VkImage& image, VkDeviceMemory& imageMemory);
            void createTextureImageView(VkImage image, VkImageView& imageView);
            void createTextureSampler(VkSampler& sampler);

            KnoxicDevice& knoxicDevice;
            MaterialProperties properties;

            // Texture resources
            VkImage albedoTextureImage = VK_NULL_HANDLE;
            VkDeviceMemory albedoTextureImageMemory = VK_NULL_HANDLE;
            VkImageView albedoTextureImageView = VK_NULL_HANDLE;
            VkSampler albedoTextureSampler = VK_NULL_HANDLE;

            VkImage normalTextureImage = VK_NULL_HANDLE;
            VkDeviceMemory normalTextureImageMemory = VK_NULL_HANDLE;
            VkImageView normalTextureImageView = VK_NULL_HANDLE;
            VkSampler normalTextureSampler = VK_NULL_HANDLE;

            VkImage roughnessTextureImage = VK_NULL_HANDLE;
            VkDeviceMemory roughnessTextureImageMemory = VK_NULL_HANDLE;
            VkImageView roughnessTextureImageView = VK_NULL_HANDLE;
            VkSampler roughnessTextureSampler = VK_NULL_HANDLE;

            VkImage metallicTextureImage = VK_NULL_HANDLE;
            VkDeviceMemory metallicTextureImageMemory = VK_NULL_HANDLE;
            VkImageView metallicTextureImageView = VK_NULL_HANDLE;
            VkSampler metallicTextureSampler = VK_NULL_HANDLE;

            // Default white texture for when no texture is loaded
            VkImage defaultTextureImage = VK_NULL_HANDLE;
            VkDeviceMemory defaultTextureImageMemory = VK_NULL_HANDLE;
            VkImageView defaultTextureImageView = VK_NULL_HANDLE;
            VkSampler defaultTextureSampler = VK_NULL_HANDLE;

            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

            bool hasAlbedoTexture = false;
            bool hasNormalTexture = false;
            bool hasRoughnessTexture = false;
            bool hasMetallicTexture = false;
    };

    struct MaterialComponent {
        std::shared_ptr<KnoxicMaterial> material;
        
        void setColor(const glm::vec3& color) { 
            if (material) material->setAlbedo(color); 
        }
        
        void loadTexture(const std::string& filepath) { 
            if (material) material->loadTexture(filepath); 
        }
        
        void setMetallic(float metallic) { 
            if (material) material->setMetallic(metallic); 
        }
        
        void setRoughness(float roughness) { 
            if (material) material->setRoughness(roughness); 
        }
    };
}