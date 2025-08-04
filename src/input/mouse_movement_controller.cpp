#include "mouse_movement_controller.hpp"

namespace knoxic {

    void MouseMovementController::init(GLFWwindow* window) {
        glfwGetCursorPos(window, &lastX, &lastY);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseHidden = true;
    }

    void MouseMovementController::updateLook(GLFWwindow* window, float dt, KnoxicGameObject& camera) {
        if (!mouseHidden) return;  // Don't rotate camera if mouse is visible

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        float xoffset = static_cast<float>(xpos - lastX);
        float yoffset = static_cast<float>(lastY - ypos);

        lastX = xpos;
        lastY = ypos;

        if (firstMouse) {
            firstMouse = false;
            return;
        }

        xoffset *= lookSpeed;
        yoffset *= lookSpeed;

        yaw += xoffset;
        pitch += yoffset;

        // Clamp pitch
        pitch = glm::clamp(pitch, -1.5f, 1.5f);
        yaw = glm::mod(yaw, glm::two_pi<float>());

        camera.transform.rotation = glm::vec3(pitch, yaw, 0.0f);
    }

    void MouseMovementController::resetCursor(GLFWwindow* window) {
        glfwGetCursorPos(window, &lastX, &lastY);
        firstMouse = true;
    }
}