#include "mouse_movement_controller.hpp"

namespace knoxic {

    void MouseMovementController::init(GLFWwindow* window) {
        // Start mouse at center of the window
        glfwGetCursorPos(window, &lastX, &lastY);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // hide and lock cursor
    }

    void MouseMovementController::updateLook(GLFWwindow* window, float dt, KnoxicGameObject& camera) {
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

        xoffset *= lookSpeed * dt;
        yoffset *= lookSpeed * dt;

        yaw += xoffset;
        pitch += yoffset;

        // Clamp pitch
        pitch = glm::clamp(pitch, glm::radians(-85.0f), glm::radians(85.0f));

        glm::vec3 direction;
        direction.x = cos(pitch) * sin(yaw);
        direction.y = sin(pitch);
        direction.z = cos(pitch) * cos(yaw);

        camera.transform.rotation = glm::vec3(glm::radians(pitch) * sensitivity, glm::radians(yaw) * sensitivity, 0.0f);
    }
}