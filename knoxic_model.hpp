#pragma once

#include "knoxic_device.hpp"

namespace knoxic {
    
    class KnoxicModel {
        public:
            KnoxicModel();
            ~KnoxicModel();

            KnoxicModel(const KnoxicModel &) = delete;
            KnoxicModel &operator=(const KnoxicModel &) = delete;

            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);

        private:
            KnoxicDevice &knoxicDevice;
            VkBuffer vertexBuffer;
            VkDeviceMemory vertexBufferMemory;
            uint32_t vertexCount;
    };
}