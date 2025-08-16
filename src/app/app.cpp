#include "app.hpp"
#include "../graphics/knoxic_frame_info.hpp"
#include "../object/knoxic_game_object.hpp"
#include "../graphics/knoxic_model.hpp"
#include "../graphics/knoxic_material.hpp"
#include "../core/knoxic_swap_chain.hpp"
#include "../camera/knoxic_camera.hpp"
#include "../input/keybord_movement_controller.hpp"
#include "../input/mouse_movement_controller.hpp"
#include "../core/knoxic_buffer.hpp"
#include "../systems/render_system.hpp"
#include "../systems/point_light_system.hpp"
#include "../systems/material_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <cassert>
#include <memory>
#include <chrono>

namespace knoxic {

    App::App() {
        globalPool = KnoxicDescriptorPool::Builder(knoxicDevice)
            .setMaxSets(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT)
            .build();

        materialPool = KnoxicDescriptorPool::Builder(knoxicDevice)
            .setMaxSets(1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4000)
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
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            );

            uboBuffers[i]->map();
        }

        auto globalSetLayout = KnoxicDescriptorSetLayout::Builder(knoxicDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        // Create material system and its descriptor set layout
        MaterialSystem materialSystem{knoxicDevice};
        auto materialSetLayout = materialSystem.createMaterialSetLayout();

        std::vector<VkDescriptorSet> globalDescriptorSets(KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            KnoxicDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        RenderSystem renderSystem {
            knoxicDevice,
            knoxicRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout(),
            materialSetLayout->getDescriptorSetLayout()
        };
        
        PointLightSystem pointLightSystem {
            knoxicDevice,
            knoxicRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        };
        
        KnoxicCamera camera{};

        auto viewerObject = KnoxicGameObject::createGameObject(knoxicDevice);
        viewerObject.transform.translation = {0.0f, 0.0f, -2.5f}; // camera position on creation
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
                    globalDescriptorSets[frameIndex],
                    gameObjects
                };

                // Update materials
                materialSystem.updateMaterials(frameInfo, *materialSetLayout, *materialPool);

                // Update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                pointLightSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // Render
                knoxicRenderer.beginSwapChainRenderPass(commandBuffer);
                renderSystem.renderGameObjects(frameInfo);
                pointLightSystem.render(frameInfo);
                knoxicRenderer.endSwapChainRenderPass(commandBuffer);
                knoxicRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(knoxicDevice.device());
    }

    void App::loadGameObjects() {
        std::shared_ptr<KnoxicModel> knoxicModel;

        // -- First scene --
        {
            // Creates the flat vase object
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/flat_vase.obj");
            auto flatVase = KnoxicGameObject::createGameObject(knoxicDevice);
            flatVase.model = knoxicModel;
            flatVase.transform.translation = {-0.5f, 0.5f, 0.0f};
            flatVase.transform.scale = {3.0f, 1.5f, 3.0f};
            gameObjects.emplace(flatVase.getId(), std::move(flatVase));

            // Creates the smooth vase object
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/smooth_vase.obj");
            auto smoothVase = KnoxicGameObject::createGameObject(knoxicDevice);
            smoothVase.model = knoxicModel;
            smoothVase.transform.translation = {0.5f, 0.5f, 0.0f};
            smoothVase.transform.scale = {3.0f, 1.5f, 3.0f};
            gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

            // Creates the floor object
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            auto floor = KnoxicGameObject::createGameObject(knoxicDevice);
            floor.model = knoxicModel;
            floor.transform.translation = {0.0f, 0.5f, 0.0f};
            floor.transform.scale = {3.0f, 1.0f, 3.0f};
            floor.material->setColor({0.9f, 0.8f, 0.4f});
            floor.material->setMetallic(0.7f);
            floor.material->setRoughness(0.3f); 
            gameObjects.emplace(floor.getId(), std::move(floor));

            // Create point lights
            {
                std::vector<glm::vec3> lightColors{
                    {1.0f, 0.1f, 0.1f},
                    {0.1f, 0.1f, 1.0f},
                    {0.1f, 1.0f, 0.1f},
                    {1.0f, 1.0f, 0.1f},
                    {0.1f, 1.0f, 1.0f},
                    {1.0f, 1.0f, 1.0f}
                };

                // Create the point lights and rotate them in a ring / circle
                for (int i = 0; i < lightColors.size(); i++) {
                    auto pointLight = KnoxicGameObject::makePointLight(knoxicDevice, 0.05f);
                    pointLight.color = lightColors[i];
                    auto rotateLight = glm::rotate(
                        glm::mat4(1.0f),
                        (i * glm::two_pi<float>()) / lightColors.size(),
                        {0.0f, -1.0f, 0.0f}
                    );
                    pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f));
                    gameObjects.emplace(pointLight.getId(), std::move(pointLight));
                }
            }
        }

        // -- Second scene --
        {
            // Creates the vase object
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/smooth_vase.fbx");
            auto vase = KnoxicGameObject::createGameObject(knoxicDevice);
            vase.model = knoxicModel;
            vase.transform.translation = {10.0f, 0.5f, 0.0f};
            vase.transform.scale = {3.0f, 1.5f, 3.0f};
            gameObjects.emplace(vase.getId(), std::move(vase));

            // Creates a point light
            auto pointLight1 = KnoxicGameObject::makePointLight(knoxicDevice, 0.05f);
            pointLight1.color = glm::vec3{1.0f, 1.0f, 1.0f};
            pointLight1.transform.translation = glm::vec3{10.0f, -0.5f, -2.0f};
            gameObjects.emplace(pointLight1.getId(), std::move(pointLight1));

            // Creates the floor object
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            auto floor2 = KnoxicGameObject::createGameObject(knoxicDevice);
            floor2.model = knoxicModel;
            floor2.transform.translation = {10.0f, 0.5f, 0.0f};
            floor2.transform.scale = {3.0f, 1.0f, 3.0f};
            floor2.material->loadTexture("res/textures/missing.png");
            floor2.material->setRoughness(0.5f);
            floor2.material->setMetallic(0.0f);
            gameObjects.emplace(floor2.getId(), std::move(floor2));
        }
    }
}