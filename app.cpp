#include "app.hpp"
#include "knoxic_descriptors.hpp"
#include "knoxic_frame_info.hpp"
#include "knoxic_game_object.hpp"
#include "knoxic_model.hpp"
#include "knoxic_swap_chain.hpp"
#include "render_system.hpp"
#include "knoxic_camera.hpp"
#include "keybord_movement_controller.hpp"
#include "mouse_movement_controller.hpp"
#include "knoxic_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <memory>
#include <chrono>

namespace knoxic {

    struct GlobalUbo {
        glm::mat4 projectionView{1.0f};
        glm::vec3 lightDirection = glm::normalize(glm::vec3{1.0f, -3.0f, 1.0f});
    };

    App::App() { 
        globalPool = KnoxicDescriptorPool::Builder(knoxicDevice)
            .setMaxSets(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

        loadGameObjects(); 
    }

    App::~App() {}

    void App::run() {
        std::vector<std::unique_ptr<KnoxicBuffer>> uboBuffers(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<KnoxicBuffer>(
                knoxicDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ,
                knoxicDevice.properties.limits.minUniformBufferOffsetAlignment
            );

            uboBuffers[i]->map();
        }

        auto globalSetLayout = KnoxicDescriptorSetLayout::Builder(knoxicDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
            .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            KnoxicDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        RenderSystem renderSystem{
            knoxicDevice,
            knoxicRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };
        KnoxicCamera camera{};

        auto viewerObject = KnoxicGameObject::createGameObject();
        MouseMovementController cameraControllerMouse{};
        KeybordMovementController cameraControllerKeybord{cameraControllerMouse};

        auto currentTime = std::chrono::high_resolution_clock::now();

        cameraControllerMouse.init(knoxicWindow.getGLFWwindow());

        while(!knoxicWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraControllerKeybord.moveInPlaneXZ(knoxicWindow.getGLFWwindow(), frameTime, viewerObject);
            cameraControllerMouse.updateLook(knoxicWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = knoxicRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.01f, 100.0f);
            
            if (auto commandBuffer = knoxicRenderer.beginFrame()) {
                int frameIndex = knoxicRenderer.getFrameIndex();
                FrameInfo frameInfo {
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex]
                };

                // Update
                GlobalUbo ubo{};
                ubo.projectionView = camera.getProjection() * camera.getView();
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // Render
                knoxicRenderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(frameInfo, gameObjects);
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