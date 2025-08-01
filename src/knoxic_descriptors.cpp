#include "knoxic_descriptors.hpp"

#include <cassert>
#include <stdexcept>

namespace knoxic {

    // -- Descriptor set layout builder --
    KnoxicDescriptorSetLayout::Builder &KnoxicDescriptorSetLayout::Builder::addBinding(
    uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t count) {
        assert(bindings.count(binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<KnoxicDescriptorSetLayout> KnoxicDescriptorSetLayout::Builder::build() const {
        return std::make_unique<KnoxicDescriptorSetLayout>(knoxicDevice, bindings);
    }

    // -- Descriptor set layout --
    KnoxicDescriptorSetLayout::KnoxicDescriptorSetLayout(KnoxicDevice &knoxicDevice,
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings) : knoxicDevice{knoxicDevice}, bindings{bindings} {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : bindings) {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(
                knoxicDevice.device(),
                &descriptorSetLayoutInfo,
                nullptr,
                &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    KnoxicDescriptorSetLayout::~KnoxicDescriptorSetLayout() {
        vkDestroyDescriptorSetLayout(knoxicDevice.device(), descriptorSetLayout, nullptr);
    }

    KnoxicDescriptorPool::Builder &KnoxicDescriptorPool::Builder::addPoolSize(
        VkDescriptorType descriptorType,
        uint32_t count) {
        poolSizes.push_back({descriptorType, count});
        return *this;
    }

    KnoxicDescriptorPool::Builder &KnoxicDescriptorPool::Builder::setPoolFlags(
        VkDescriptorPoolCreateFlags flags) {
        poolFlags = flags;
        return *this;
    }

    KnoxicDescriptorPool::Builder &KnoxicDescriptorPool::Builder::setMaxSets(uint32_t count) {
        maxSets = count;
        return *this;
    }

    std::unique_ptr<KnoxicDescriptorPool> KnoxicDescriptorPool::Builder::build() const {
        return std::make_unique<KnoxicDescriptorPool>(knoxicDevice, maxSets, poolFlags, poolSizes);
    }

    // -- Descriptor pool --
    KnoxicDescriptorPool::KnoxicDescriptorPool(KnoxicDevice &knoxicDevice,  uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
    const std::vector<VkDescriptorPoolSize> &poolSizes) : knoxicDevice{knoxicDevice} {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(knoxicDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    KnoxicDescriptorPool::~KnoxicDescriptorPool() {
        vkDestroyDescriptorPool(knoxicDevice.device(), descriptorPool, nullptr);
    }

    bool KnoxicDescriptorPool::allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(knoxicDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS) {
            return false;
        }

        return true;
    }

    void KnoxicDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const {
        vkFreeDescriptorSets(
            knoxicDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data()
        );
    }

    void KnoxicDescriptorPool::resetPool() {
        vkResetDescriptorPool(knoxicDevice.device(), descriptorPool, 0);
    }

    // -- Descriptor writer --
    KnoxicDescriptorWriter::KnoxicDescriptorWriter(KnoxicDescriptorSetLayout &setLayout, KnoxicDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}

    KnoxicDescriptorWriter &KnoxicDescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto &bindingDescription = setLayout.bindings[binding];

        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    KnoxicDescriptorWriter &KnoxicDescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo) {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto &bindingDescription = setLayout.bindings[binding];

        assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    bool KnoxicDescriptorWriter::build(VkDescriptorSet &set) {
        bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
        if (!success) {
            return false;
        }
        overwrite(set);
        return true;
    }

    void KnoxicDescriptorWriter::overwrite(VkDescriptorSet &set) {
        for (auto &write : writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(pool.knoxicDevice.device(), writes.size(), writes.data(), 0, nullptr);
    }
}