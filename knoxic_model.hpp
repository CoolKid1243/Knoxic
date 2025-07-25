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
                glm::vec3 position;
                glm::vec3 color;

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
            };

            struct Data {
                std::vector<Vertex> vertices{};
                std::vector<uint32_t> indices{};
            };

            KnoxicModel(KnoxicDevice &device, const KnoxicModel::Data &data);
            ~KnoxicModel();

            KnoxicModel(const KnoxicModel &) = delete;
            KnoxicModel &operator=(const KnoxicModel &) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);
            void createIndexBuffer(const std::vector<uint32_t> &indices);

            KnoxicDevice &knoxicDevice;

            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;

            bool hasIndexBuffer = false;
            VkBuffer indexBuffer;
            VkDeviceMemory indexBufferMemory;
            uint32_t indexCount;
    };
}