#pragma once

#include "knoxic_model.hpp"

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

            static KnoxicGameObject createGameObject() {
                static id_t currentId = 0;
                return KnoxicGameObject(currentId++);
            }

            static KnoxicGameObject makePointLight(float intensity = 10.0f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.0f));

            KnoxicGameObject(const KnoxicGameObject &) = delete;
            KnoxicGameObject &operator=(const KnoxicGameObject &) = delete;
            KnoxicGameObject(KnoxicGameObject &&) = default;
            KnoxicGameObject &operator=(KnoxicGameObject &&) = default;

            const id_t getId() { return id; };

            glm::vec3 color{};
            TransformComponent transform{};
            
            std::shared_ptr<KnoxicModel> model{};
            std::unique_ptr<PointLightComponent> pointLight = nullptr;

        private:
            KnoxicGameObject(id_t objId) : id{objId} {}

            id_t id;
    };
}