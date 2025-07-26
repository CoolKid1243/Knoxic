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

    void App::loadGameObjects() {
        std::shared_ptr<KnoxicModel> knoxicModel;

        // Creates the flat vase object
        knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/flat_vase.obj");
        auto flatVase = KnoxicGameObject::createGameObject();
        flatVase.model = knoxicModel;
        flatVase.transform.translation = {-0.5f, 0.5f, 2.5f};
        flatVase.transform.scale = {3.0f, 1.5f, 3.0f};
        gameObjects.push_back(std::move(flatVase));

        // Creates the smooth vase object
        knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/smooth_vase.obj");
        auto smoothVase = KnoxicGameObject::createGameObject();
        smoothVase.model = knoxicModel;
        smoothVase.transform.translation = {0.5f, 0.5f, 2.5f};
        smoothVase.transform.scale = {3.0f, 1.5f, 3.0f};
        gameObjects.push_back(std::move(smoothVase));
    }
}