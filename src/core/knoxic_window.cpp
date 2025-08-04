#include "knoxic_window.hpp"

#include <stdexcept>

namespace knoxic {

    KnoxicWindow::KnoxicWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name} {
        initWindow();
    }

    KnoxicWindow::~KnoxicWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void KnoxicWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
    }

    void KnoxicWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface");
        }
    }

    void KnoxicWindow::framebufferResizedCallback(GLFWwindow *window, int width, int height) {
        auto knoxicWindow = reinterpret_cast<KnoxicWindow *>(glfwGetWindowUserPointer(window));
        knoxicWindow->framebufferResized = true;
        knoxicWindow->width = width;
        knoxicWindow->height = height;
    }
}