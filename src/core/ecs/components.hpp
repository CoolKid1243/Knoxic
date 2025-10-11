#pragma once

#include "../../graphics/vulkan/knoxic_vk_model.hpp"
#include "../../graphics/vulkan/knoxic_vk_material.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>

namespace knoxic {

    // Core transform component
    struct TransformComponent {
        glm::vec3 translation{}; // position offset
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rotation{};

        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };

    // Simple color component used when no material is bound
    struct ColorComponent {
        glm::vec3 color{1.0f, 1.0f, 1.0f};
    };

    // Tag/data for point lights
    struct PointLightComponent {
        float lightIntensity = 1.0f;
    };

    // Reference to a renderable model
    struct ModelComponent {
        std::shared_ptr<KnoxicModel> model{};
    };

    using MaterialComponent = ::knoxic::MaterialComponent;
}