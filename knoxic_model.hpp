#pragma once

#include "knoxic_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>

namespace knoxic {
    
    class KnoxicModel {
        public:
            struct Vertex {
                glm::vec2 positon;

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };

            KnoxicModel(KnoxicDevice &device, const std::vector<Vertex> &vertices);
            ~KnoxicModel();

            KnoxicModel(const KnoxicModel &) = delete;
            KnoxicModel &operator=(const KnoxicModel &) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);

            KnoxicDevice &knoxicDevice;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;
    };
}