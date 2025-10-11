#include "knoxic_vk_material_system.hpp"
#include "../../core/ecs/components.hpp"
#include "../../core/ecs/coordinator_instance.hpp"

namespace knoxic {

    MaterialSystem::MaterialSystem(KnoxicDevice &device) : knoxicDevice{device} {}

    MaterialSystem::~MaterialSystem() {}

    std::unique_ptr<KnoxicDescriptorSetLayout> MaterialSystem::createMaterialSetLayout() {
        return KnoxicDescriptorSetLayout::Builder(knoxicDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Albedo
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Normal
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Roughness
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Metallic
            .build();
    }

    void MaterialSystem::updateMaterials(
        FrameInfo & /*frameInfo*/,
        KnoxicDescriptorSetLayout& materialSetLayout,
        KnoxicDescriptorPool& materialPool,
        std::shared_ptr<RenderableSystem> renderableSystem
    ) {
        // Iterate over ECS renderable entities and update material descriptor sets
        for (auto entity : renderableSystem->mEntities) {
            if (gCoordinator.HasComponent<MaterialComponent>(entity)) {
                auto &matComp = gCoordinator.GetComponent<MaterialComponent>(entity);
                if (matComp.material) {
                    matComp.material->updateDescriptorSet(materialSetLayout, materialPool);
                }
            }
        }
    }
}