#include "app.hpp"
#include "knoxic_device.hpp"
#include "knoxic_game_object.hpp"
#include "knoxic_model.hpp"
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

    std::unique_ptr<KnoxicModel> createCubeModel(KnoxicDevice& device, glm::vec3 offset) {
        std::vector<KnoxicModel::Vertex> vertices{
            // left face (white)
            {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, 0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, -0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, 0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, 0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}},
        
            // right face (yellow)
            {{0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, -0.5f, 0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.1f}},
        
            // top face (orange)
            {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.6f, 0.1f}},
            {{0.5f, -0.5f, 0.5f}, {0.9f, 0.6f, 0.1f}},
            {{-0.5f, -0.5f, 0.5f}, {0.9f, 0.6f, 0.1f}},
            {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.6f, 0.1f}},
            {{0.5f, -0.5f, -0.5f}, {0.9f, 0.6f, 0.1f}},
            {{0.5f, -0.5f, 0.5f}, {0.9f, 0.6f, 0.1f}},
        
            // bottom face (red)
            {{-0.5f, 0.5f, -0.5f}, {0.8f, 0.1f, 0.1f}},
            {{0.5f, 0.5f, 0.5f}, {0.8f, 0.1f, 0.1f}},
            {{-0.5f, 0.5f, 0.5f}, {0.8f, 0.1f, 0.1f}},
            {{-0.5f, 0.5f, -0.5f}, {0.8f, 0.1f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.8f, 0.1f, 0.1f}},
            {{0.5f, 0.5f, 0.5f}, {0.8f, 0.1f, 0.1f}},
        
            // nose face (blue)
            {{-0.5f, -0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{-0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{-0.5f, -0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{0.5f, -0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
        
            // tail face (green)
            {{-0.5f, -0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{-0.5f, 0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{-0.5f, -0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{0.5f, -0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
        
        };

        for (auto& v : vertices) {
            v.position += offset;
        }

        return std::make_unique<KnoxicModel>(device, vertices);
    }

    void App::loadGameObjects() {
        std::shared_ptr<KnoxicModel> knoxicModel = createCubeModel(knoxicDevice, {0.0f, 0.0f, 0.0f});

        auto cube = KnoxicGameObject::createGameObject();
        cube.model = knoxicModel;
        cube.transform.translation = {0.0f, 0.0f, 0.5f};
        cube.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back(std::move(cube));
    }
}