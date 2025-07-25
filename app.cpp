#include "app.hpp"
#include "knoxic_game_object.hpp"
#include "knoxic_model.hpp"
#include "render_system.hpp"
#include "knoxic_camera.hpp"
#include "keybord_movement_controller.hpp"
#include "mouse_movement_controller.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <memory>
#include <chrono>

namespace knoxic {

    App::App() {
        loadGameObjects();
    }

    App::~App() {}

    void App::run() {
        RenderSystem renderSystem{knoxicDevice, knoxicRenderer.getSwapChainRenderPass()};
        KnoxicCamera camera{};

        auto viewerObject = KnoxicGameObject::createGameObject();
        KeybordMovementController cameraController{};
        MouseMovementController mouseController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

        mouseController.init(knoxicWindow.getGLFWwindow());

        while(!knoxicWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(knoxicWindow.getGLFWwindow(), frameTime, viewerObject);
            mouseController.updateLook(knoxicWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = knoxicRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.01f, 100.0f);
            
            if (auto commandBuffer = knoxicRenderer.beginFrame()) {
                knoxicRenderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
                knoxicRenderer.endSwapChainRenderPass(commandBuffer);
                knoxicRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(knoxicDevice.device());
    }

    std::unique_ptr<KnoxicModel> createCubeModel(KnoxicDevice& device, glm::vec3 offset) {
        KnoxicModel::Data modelData{};
        modelData.vertices  = {
            // Left face (white)
            {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, 0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, -0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}},
            {{-0.5f, 0.5f, -0.5f}, {0.9f, 0.9f, 0.9f}},
        
            // Right face (yellow)
            {{0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, 0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, -0.5f, 0.5f}, {0.8f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.8f, 0.8f, 0.1f}},

            // Top face
            {{-0.5f, -0.5f, -0.5f}, {0.9f, 0.6f, 0.1f}},
            {{0.5f, -0.5f, 0.5f}, {0.9f, 0.6f, 0.1f}},
            {{-0.5f, -0.5f, 0.5f}, {0.9f, 0.6f, 0.1f}},
            {{0.5f, -0.5f, -0.5f}, {0.9f, 0.6f, 0.1f}},

            // Bottom face (red)
            {{-0.5f, 0.5f, -0.5f}, {0.8f, 0.1f, 0.1f}},
            {{0.5f, 0.5f, 0.5f}, {0.8f, 0.1f, 0.1f}},
            {{-0.5f, 0.5f, 0.5f}, {0.8f, 0.1f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.8f, 0.1f, 0.1f}},

            // Front face (blue)
            {{-0.5f, -0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{-0.5f, 0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},
            {{0.5f, -0.5f, 0.5f}, {0.1f, 0.1f, 0.8f}},

            // Back face (green)
            {{-0.5f, -0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{0.5f, 0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{-0.5f, 0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
            {{0.5f, -0.5f, -0.5f}, {0.1f, 0.8f, 0.1f}},
        };
        for (auto& v : modelData.vertices) {
            v.position += offset;
        }

        modelData.indices = {
            0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
            12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21
        };

        return std::make_unique<KnoxicModel>(device, modelData);
    }

    void App::loadGameObjects() {
        // Creates the cube
        std::shared_ptr<KnoxicModel> knoxicModel = createCubeModel(knoxicDevice, {0.0f, 0.0f, 0.0f});
        auto cube = KnoxicGameObject::createGameObject();
        cube.model = knoxicModel;
        cube.transform.translation = {0.0f, 0.0f, 2.5f};
        cube.transform.scale = {0.5f, 0.5f, 0.5f};
        gameObjects.push_back(std::move(cube));
    }
}