#pragma once

#include "../object/knoxic_game_object.hpp"
#include "../input/mouse_movement_controller.hpp"

namespace knoxic {
    
    class KeybordMovementController {
    public:
        struct KeyMappings {
            int moveLeft = GLFW_KEY_A;
            int moveRight = GLFW_KEY_D;
            int moveForward = GLFW_KEY_W;
            int moveBackward = GLFW_KEY_S;
            int moveUp = GLFW_KEY_E;
            int moveDown = GLFW_KEY_Q;

            int esc = GLFW_KEY_ESCAPE;
            int tab = GLFW_KEY_TAB;
        };

        KeybordMovementController(MouseMovementController& mc) : mouseController(mc) {}

        void moveInPlaneXZ(GLFWwindow *window, float dt, KnoxicGameObject &gameObject);

        KeyMappings keys{};
        float moveSpeed{3.0f};
        float lookSpeed{1.5f};
        bool mouseHidden = true;

    private:
        MouseMovementController &mouseController;
    };
}