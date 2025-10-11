#pragma once

#include "../../core/vulkan/knoxic_vk_device.hpp"
#include "../../core/vulkan/knoxic_vk_descriptors.hpp"
#include "../../graphics/knoxic_frame_info.hpp"
#include "../../core/ecs/ecs_systems.hpp"

#include <memory>

namespace knoxic {

    class MaterialSystem {
    public:
        MaterialSystem(KnoxicDevice &device);
        ~MaterialSystem();

        MaterialSystem(const MaterialSystem &) = delete;
        MaterialSystem &operator=(const MaterialSystem &) = delete;

        std::unique_ptr<KnoxicDescriptorSetLayout> createMaterialSetLayout();
            
        void updateMaterials(FrameInfo &frameInfo, KnoxicDescriptorSetLayout& materialSetLayout, KnoxicDescriptorPool& materialPool, 
            std::shared_ptr<RenderableSystem> renderableSystem);

    private:
        KnoxicDevice &knoxicDevice;
    };
}