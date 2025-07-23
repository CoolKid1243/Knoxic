#pragma once

#include "knoxic_model.hpp"

#include <memory>

namespace knoxic {

    struct Transform2dComponent {
        glm::vec2 translation{}; // position offset
        glm::vec2 scale{1.0f, 1.0f};
        float rotation;

        glm::mat2 mat2() { 
            const float s = glm::sin(rotation);
            const float c = glm::cos(rotation);
            glm::mat2 rotMatrix{{c, s}, {-s, c}};

            glm::mat2 scaleMat{{scale.x, .0f}, {.0f, scale.y}};
            return rotMatrix * scaleMat;
        }
    };

    class KnoxicGameObject {
        public:
            using id_t = unsigned int;

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

            Transform2dComponent transform2d{};

        private:
            KnoxicGameObject(id_t objId) : id{objId} {}

            id_t id;
    };
}