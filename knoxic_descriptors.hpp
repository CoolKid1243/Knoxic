#pragma once

#include "knoxic_device.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace knoxic {

    class KnoxicDescriptorSetLayout {
        public:
            class Builder {
                public:
                    Builder(KnoxicDevice &knoxicDevice) : knoxicDevice{knoxicDevice} {}

                    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count = 1);
                    std::unique_ptr<KnoxicDescriptorSetLayout> build() const;

                private:
                    KnoxicDevice &knoxicDevice;
                    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
            };

            KnoxicDescriptorSetLayout(KnoxicDevice &knoxicDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
            ~KnoxicDescriptorSetLayout();

            KnoxicDescriptorSetLayout(const KnoxicDescriptorSetLayout &) = delete;
            KnoxicDescriptorSetLayout &operator=(const KnoxicDescriptorSetLayout &) = delete;

            VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

        private:
            KnoxicDevice &knoxicDevice;
            VkDescriptorSetLayout descriptorSetLayout;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

            friend class KnoxicDescriptorWriter;
    };

    class KnoxicDescriptorPool {
        public:
            class Builder {
                public:
                    Builder(KnoxicDevice &knoxicDevice) : knoxicDevice{knoxicDevice} {}

                    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
                    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
                    Builder &setMaxSets(uint32_t count);
                    std::unique_ptr<KnoxicDescriptorPool> build() const;

                private:
                    KnoxicDevice &knoxicDevice;
                    std::vector<VkDescriptorPoolSize> poolSizes{};
                    uint32_t maxSets = 1000;
                    VkDescriptorPoolCreateFlags poolFlags = 0;
            };

            KnoxicDescriptorPool(
                KnoxicDevice &knoxicDevice,
                uint32_t maxSets,
                VkDescriptorPoolCreateFlags poolFlags,
                const std::vector<VkDescriptorPoolSize> &poolSizes);
            ~KnoxicDescriptorPool();

            KnoxicDescriptorPool(const KnoxicDescriptorPool &) = delete;
            KnoxicDescriptorPool &operator=(const KnoxicDescriptorPool &) = delete;

            bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
            void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;
            void resetPool();

        private:
            KnoxicDevice &knoxicDevice;
            VkDescriptorPool descriptorPool;

            friend class KnoxicDescriptorWriter;
    };

    class KnoxicDescriptorWriter {
        public:
            KnoxicDescriptorWriter(KnoxicDescriptorSetLayout &setLayout, KnoxicDescriptorPool &pool);

            KnoxicDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
            KnoxicDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

            bool build(VkDescriptorSet &set);
            void overwrite(VkDescriptorSet &set);

        private:
            KnoxicDescriptorSetLayout &setLayout;
            KnoxicDescriptorPool &pool;
            std::vector<VkWriteDescriptorSet> writes;
    };
}