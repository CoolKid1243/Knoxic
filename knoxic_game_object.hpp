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

    class KnoxicGameObject {
        public:
            using id_t = unsigned int;
            using Map = std::unordered_map<id_t, KnoxicGameObject>;

            static KnoxicGameObject createGameObject() {
                static id_t currentId = 0;
                return KnoxicGameObject(currentId++);
            }

            KnoxicGameObject(const KnoxicGameObject &) = delete;
            KnoxicGameObject &operator=(const KnoxicGameObject &) = delete;
            KnoxicGameObject(KnoxicGameObject &&) = default;
            KnoxicGameObject &operator=(KnoxicGameObject &&) = default;

            const id_t getId() { return id; };

            std::shared_ptr<KnoxicModel> model{};
            glm::vec3 color{};

            TransformComponent transform{};

        private:
            KnoxicGameObject(id_t objId) : id{objId} {}

            id_t id;
    };
}