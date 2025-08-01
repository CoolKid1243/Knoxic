#pragma once

#include "knoxic_device.hpp"
#include "knoxic_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace knoxic {
    
    class KnoxicModel {
        public:
            struct Vertex {
                glm::vec3 position{};
                glm::vec3 color{};
                glm::vec3 normal{};
                glm::vec2 uv{};

                static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
                static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

                bool operator==(const Vertex &other) const { 
                    return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
                }
            };

            struct Data {
                std::vector<Vertex> vertices{};
                std::vector<uint32_t> indices{};

                void loadModel(const std::string &filePath);
            };

            KnoxicModel(KnoxicDevice &device, const KnoxicModel::Data &data);
            ~KnoxicModel();

            KnoxicModel(const KnoxicModel &) = delete;
            KnoxicModel &operator=(const KnoxicModel &) = delete;

            static std::unique_ptr<KnoxicModel> createModelFromFile(KnoxicDevice &device, const std::string &filePath);

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            void createVertexBuffers(const std::vector<Vertex> &vertices);
            void createIndexBuffer(const std::vector<uint32_t> &indices);

            KnoxicDevice &knoxicDevice;

            std::unique_ptr<KnoxicBuffer> vertexBuffer;
            uint32_t vertexCount;

            bool hasIndexBuffer = false;
            std::unique_ptr<KnoxicBuffer> indexBuffer;
            VkDeviceMemory indexBufferMemory;
            uint32_t indexCount;
    };
}