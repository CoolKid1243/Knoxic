#pragma once

#include "knoxic_game_object.hpp"
#include <GLFW/glfw3.h>

namespace knoxic {

    class MouseMovementController {
        public:
            void init(GLFWwindow* window);
            void updateLook(GLFWwindow* window, float dt, KnoxicGameObject& camera);

            float lookSpeed = 0.1f;
            bool firstMouse = true;

        private:
            float sensitivity = 50.0f;
            double lastX = 0.0f;
            double lastY = 0.0f;
            float pitch = 0.0f;
            float yaw = 0.0f;
    };
}