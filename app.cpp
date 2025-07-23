#include "app.hpp"
#include "knoxic_device.hpp"
#include "render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <memory>

namespace knoxic {

    App::App() {
        loadGameObjects();
    }

    App::~App() {}

    void App::run() {
        RenderSystem renderSystem{knoxicDevice, knoxicRenderer.getSwapChainRenderPass()};

        while(!knoxicWindow.shouldClose()) {
            glfwPollEvents();
            
            if (auto commandBuffer = knoxicRenderer.beginFrame()) {
                knoxicRenderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(commandBuffer, gameObjects);
                knoxicRenderer.endSwapChainRenderPass(commandBuffer);
                knoxicRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(knoxicDevice.device());
    }

    void App::loadGameObjects() {
        std::vector<KnoxicModel::Vertex> vertices{
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
        };
        auto knoxicModel = std::make_shared<KnoxicModel>(knoxicDevice, vertices);

        auto triangle = KnoxicGameObject::createGameObject();
        triangle.model = knoxicModel;
        triangle.color = {0.1f, 0.8f, 0.1f};
        triangle.transform2d.translation.x = 0.2f;
        triangle.transform2d.scale = {2.0f, 0.5f};
        triangle.transform2d.rotation = 0.25f * glm::two_pi<float>();

        gameObjects.push_back(std::move(triangle));
    }
}