#include "knoxic_buffer.hpp"
 
#include <cassert>
#include <cstring>

namespace knoxic {

    VkDeviceSize KnoxicBuffer::getAlignment(VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment) {
        if (minOffsetAlignment > 0) {
            return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
        }
        return instanceSize;
    }

    KnoxicBuffer::KnoxicBuffer(KnoxicDevice &device, VkDeviceSize instanceSize, uint32_t instanceCount,
    VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,  VkDeviceSize minOffsetAlignment) : knoxicDevice{device},
    instanceSize{instanceSize}, instanceCount{instanceCount}, usageFlags{usageFlags}, memoryPropertyFlags{memoryPropertyFlags} {
        alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
        bufferSize = alignmentSize * instanceCount;
        device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer, memory);
    }

    KnoxicBuffer::~KnoxicBuffer() {
        unmap();
        vkDestroyBuffer(knoxicDevice.device(), buffer, nullptr);
        vkFreeMemory(knoxicDevice.device(), memory, nullptr);
    }

    VkResult KnoxicBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
        assert(buffer && memory && "Called map on buffer before create");
        return vkMapMemory(knoxicDevice.device(), memory, offset, size, 0, &mapped);
    }

    void KnoxicBuffer::unmap() {
        if (mapped) {
            vkUnmapMemory(knoxicDevice.device(), memory);
            mapped = nullptr;
        }
    }

    void KnoxicBuffer::writeToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
        assert(mapped && "Cannot copy to unmapped buffer");
    
        if (size == VK_WHOLE_SIZE) {
            memcpy(mapped, data, bufferSize);
        } else {
            char *memOffset = (char *)mapped;
            memOffset += offset;
            memcpy(memOffset, data, size);
        }
    }

    VkResult KnoxicBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkFlushMappedMemoryRanges(knoxicDevice.device(), 1, &mappedRange);
    }

    VkResult KnoxicBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
        VkMappedMemoryRange mappedRange = {};
        mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedRange.memory = memory;
        mappedRange.offset = offset;
        mappedRange.size = size;
        return vkInvalidateMappedMemoryRanges(knoxicDevice.device(), 1, &mappedRange);
    }

    VkDescriptorBufferInfo KnoxicBuffer::descriptorInfo(VkDeviceSize size, VkDeviceSize offset) {
        return VkDescriptorBufferInfo{ buffer, offset, size };
    }

    void KnoxicBuffer::writeToIndex(void *data, int index) {
        writeToBuffer(data, instanceSize, index * alignmentSize);
    }

    VkResult KnoxicBuffer::flushIndex(int index) { return flush(alignmentSize, index * alignmentSize); }

    VkDescriptorBufferInfo KnoxicBuffer::descriptorInfoForIndex(int index) {
        return descriptorInfo(alignmentSize, index * alignmentSize);
    }

    VkResult KnoxicBuffer::invalidateIndex(int index) {
        return invalidate(alignmentSize, index * alignmentSize);
    }
}