#include "keybord_movement_controller.hpp"

#include "GLFW/glfw3.h"

namespace knoxic {

    void KeybordMovementController::moveInPlaneXZ(GLFWwindow *window, float dt, KnoxicGameObject &gameObject) {
        // Calculate the directions
        float yaw = gameObject.transform.rotation.y;
        const glm::vec3 forwardDir{sin(yaw), 0.0f, cos(yaw)};
        const glm::vec3 rightDir{cos(yaw), 0.f, -sin(yaw)};
        const glm::vec3 upDir{0.f, -1.f, 0.f};

        // Movement
        glm::vec3 moveDir{0.f};
        if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
        if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
        if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
        if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
        if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
        if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

        if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
            gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
        }

        // Close the window with (ESC)
        if (glfwGetKey(window, keys.esc) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Toggle mouse with (TAB)
        static bool tabPressedLastFrame = false;
        if (glfwGetKey(window, keys.tab) == GLFW_PRESS) {
            if (!tabPressedLastFrame) {
                mouseHidden = !mouseHidden;
                mouseController.mouseHidden = mouseHidden;

                glfwSetInputMode(window, GLFW_CURSOR, mouseHidden ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
                mouseController.resetCursor(window);
            }
            tabPressedLastFrame = true;
        } else {
            tabPressedLastFrame = false;
        }
    }
}