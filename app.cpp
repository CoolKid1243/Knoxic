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
        // Creates the game object
        std::shared_ptr<KnoxicModel> knoxicModel = KnoxicModel::createModelFromFile(
            knoxicDevice, "res/models/smooth_vase.obj"
        );
        auto gameObj = KnoxicGameObject::createGameObject();
        gameObj.model = knoxicModel;
        gameObj.transform.translation = {0.0f, 0.0f, 2.5f};
        gameObj.transform.scale = glm::vec3{3.0f};
        gameObjects.push_back(std::move(gameObj));
    }
}