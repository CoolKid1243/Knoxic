#pragma once

#include "../core/knoxic_game_object.hpp"

namespace knoxic {

    class MouseMovementController {
    public:
        void init(GLFWwindow* window);
        void updateLook(GLFWwindow* window, float dt, KnoxicGameObject& camera);

        void resetCursor(GLFWwindow* window);

        bool mouseHidden = true;

        float lookSpeed = 0.0025f;
        bool firstMouse = true;

    private:
        float sensitivity = 50.0f;
        double lastX = 0.0f;
        double lastY = 0.0f;
        float pitch = 0.0f;
        float yaw = 0.0f;
    };
}