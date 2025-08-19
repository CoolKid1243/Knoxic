#pragma once

#include "../graphics/vulkan/knoxic_vk_model.hpp"
#include "../graphics/vulkan/knoxic_vk_material.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <unordered_map>

namespace knoxic {

    struct TransformComponent {
        glm::vec3 translation{}; // position offset
        glm::vec3 scale{1.0f, 1.0f, 1.0f};
        glm::vec3 rotation{};

        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };

    struct PointLightComponent {
        float lightIntensity = 1.0f;
    };

    class KnoxicGameObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, KnoxicGameObject>;

        static KnoxicGameObject createGameObject(KnoxicDevice& device) {
            static id_t currentId = 0;
            return KnoxicGameObject(currentId++, device);
        }

        static KnoxicGameObject makePointLight(KnoxicDevice& device, float intensity = 10.0f, float radius = 0.1f,
        glm::vec3 color = glm::vec3(1.0f)) {
            KnoxicGameObject gameObj = createGameObject(device);
            gameObj.color = color;
            gameObj.transform.scale = glm::vec3(radius);
            gameObj.pointLight = std::make_unique<PointLightComponent>();
            gameObj.pointLight->lightIntensity = intensity;
            return gameObj;
        }

        KnoxicGameObject(const KnoxicGameObject&) = delete;
        KnoxicGameObject& operator=(const KnoxicGameObject&) = delete;
        KnoxicGameObject(KnoxicGameObject&&) = default;
        KnoxicGameObject& operator=(KnoxicGameObject&&) = default;

        const id_t getId() { return id; };

        glm::vec3 color{};
        TransformComponent transform{};
        
        std::shared_ptr<KnoxicModel> model{};
        std::unique_ptr<PointLightComponent> pointLight = nullptr;
        std::unique_ptr<MaterialComponent> material;

    private:
        KnoxicGameObject(id_t objId, KnoxicDevice& device) : id{objId} {
            material = std::make_unique<MaterialComponent>();
            material->material = std::make_shared<KnoxicMaterial>(device);
        }

        id_t id;
    };
}