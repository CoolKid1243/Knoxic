#include "app.hpp"
#include "../graphics/knoxic_frame_info.hpp"
#include "../core/knoxic_game_object.hpp"
#include "../graphics/vulkan/knoxic_vk_model.hpp"
#include "../graphics/vulkan/knoxic_vk_material.hpp"
#include "../core/vulkan/knoxic_vk_swap_chain.hpp"
#include "../camera/knoxic_camera.hpp"
#include "../input/keybord_movement_controller.hpp"
#include "../input/mouse_movement_controller.hpp"
#include "../core/vulkan/knoxic_vk_buffer.hpp"
#include "../systems/vulkan/knoxic_vk_render_system.hpp"
#include "../systems/vulkan/knoxic_vk_point_light_system.hpp"
#include "../systems/vulkan/knoxic_vk_spot_light_system.hpp"
#include "../systems/vulkan/knoxic_vk_directional_light_system.hpp"
#include "../systems/vulkan/knoxic_vk_material_system.hpp"
#include "../systems/knoxic_editor_system.hpp"
#include "../core/ecs/coordinator_instance.hpp"
#include "../core/ecs/components.hpp"
#include "../core/ecs/ecs_systems.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

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

        // Create ImGui descriptor pool
        imguiPool = KnoxicDescriptorPool::Builder(knoxicDevice)
            .setMaxSets(1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000)
            .addPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000)
            .build();

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // Viewports disabled for now - requires additional Vulkan setup for multi-viewport support
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        // if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //     style.WindowRounding = 0.0f;
        //     style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        // }

        // Setup Platform backend (Vulkan backend will be initialized in run() after swap chain is created)
        ImGui_ImplGlfw_InitForVulkan(knoxicWindow.getGLFWwindow(), true);

        // Create post-processing system
        VkExtent2D extent = knoxicRenderer.getSwapChainExtent();
        postProcessSystem = std::make_unique<PostProcessSystem>(knoxicDevice, extent);

        // Initialize ECS and register components/systems
        gCoordinator.Init();
        gCoordinator.RegisterComponent<TransformComponent>();
        gCoordinator.RegisterComponent<ModelComponent>();
        gCoordinator.RegisterComponent<MaterialComponent>();
        gCoordinator.RegisterComponent<ColorComponent>();
        gCoordinator.RegisterComponent<PointLightComponent>();
        gCoordinator.RegisterComponent<SpotLightComponent>();
        gCoordinator.RegisterComponent<DirectionalLightComponent>();
        gCoordinator.RegisterComponent<PostProcessingComponent>();

        renderableSystem = gCoordinator.RegisterSystem<RenderableSystem>();
        {
            Signature signature;
            signature.set(gCoordinator.GetComponentType<TransformComponent>());
            signature.set(gCoordinator.GetComponentType<ModelComponent>());
            gCoordinator.SetSystemSignature<RenderableSystem>(signature);
        }

        pointLightSystem = gCoordinator.RegisterSystem<PointLightECSSystem>();
        {
            Signature signature;
            signature.set(gCoordinator.GetComponentType<TransformComponent>());
            signature.set(gCoordinator.GetComponentType<PointLightComponent>());
            gCoordinator.SetSystemSignature<PointLightECSSystem>(signature);
        }

        spotLightSystem = gCoordinator.RegisterSystem<SpotLightECSSystem>();
        {
            Signature signature;
            signature.set(gCoordinator.GetComponentType<TransformComponent>());
            signature.set(gCoordinator.GetComponentType<SpotLightComponent>());
            gCoordinator.SetSystemSignature<SpotLightECSSystem>(signature);
        }

        directionalLightSystem = gCoordinator.RegisterSystem<DirectionalLightECSSystem>();
        {
            Signature signature;
            signature.set(gCoordinator.GetComponentType<TransformComponent>());
            signature.set(gCoordinator.GetComponentType<DirectionalLightComponent>());
            gCoordinator.SetSystemSignature<DirectionalLightECSSystem>(signature);
        }

        loadGameObjects(); 
    }

    App::~App() {
        // Cleanup ImGui
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Ensure GPU resources inside ECS components are destroyed before Vulkan device
        if (renderableSystem) {
            for (auto entity : renderableSystem->mEntities) {
                // Release materials
                if (gCoordinator.HasComponent<MaterialComponent>(entity)) {
                    auto &matComp = gCoordinator.GetComponent<MaterialComponent>(entity);
                    if (matComp.material) {
                        matComp.material.reset();
                    }
                }
                // Release models (buffers)
                if (gCoordinator.HasComponent<ModelComponent>(entity)) {
                    auto &modelComp = gCoordinator.GetComponent<ModelComponent>(entity);
                    if (modelComp.model) {
                        modelComp.model.reset();
                    }
                }
            }
        }
    }

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

        // Initialize ImGui Vulkan backend
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = VK_API_VERSION_1_0;
        init_info.Instance = knoxicDevice.getInstance();
        init_info.PhysicalDevice = knoxicDevice.getPhysicalDevice();
        init_info.Device = knoxicDevice.device();
        init_info.QueueFamily = knoxicDevice.findPhysicalQueueFamilies().graphicsFamily;
        init_info.Queue = knoxicDevice.graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imguiPool->getPool();
        init_info.DescriptorPoolSize = 0; // Using external pool
        init_info.MinImageCount = KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = KnoxicSwapChain::MAX_FRAMES_IN_FLIGHT;
        init_info.UseDynamicRendering = false;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        init_info.PipelineInfoMain.RenderPass = knoxicRenderer.getSwapChainRenderPass();
        init_info.PipelineInfoMain.Subpass = 0;
        init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        ImGui_ImplVulkan_Init(&init_info);

        RenderSystem renderSystem {
            knoxicDevice,
            postProcessSystem->getHDRRenderPass(),
            globalSetLayout->getDescriptorSetLayout(),
            materialSetLayout->getDescriptorSetLayout(),
            renderableSystem
        };
        
        PointLightSystem pointLightVkSystem {
            knoxicDevice,
            postProcessSystem->getHDRRenderPass(),
            globalSetLayout->getDescriptorSetLayout(),
            pointLightSystem
        };
        
        SpotLightSystem spotLightVkSystem {
            knoxicDevice,
            postProcessSystem->getHDRRenderPass(),
            globalSetLayout->getDescriptorSetLayout(),
            spotLightSystem
        };
        
        DirectionalLightSystem directionalLightVkSystem {
            knoxicDevice,
            postProcessSystem->getHDRRenderPass(),
            globalSetLayout->getDescriptorSetLayout(),
            directionalLightSystem
        };
        
        KnoxicCamera camera{};

        // Create camera ECS entity
        cameraEntity = gCoordinator.CreateEntity();
        TransformComponent cameraTransform{};
        cameraTransform.translation = {0.0f, 0.0f, -2.5f};
        gCoordinator.AddComponent(cameraEntity, cameraTransform);
        PostProcessingComponent postProc{};
        postProc.bloomEnabled = true;
        postProc.bloomThreshold = 0.8f;
        postProc.bloomIntensity = 1.2f;
        postProc.bloomIterations = 5;
        postProc.exposure = 1.0f;
        postProc.gamma = 1.65f;
        gCoordinator.AddComponent(cameraEntity, postProc);
        KnoxicGameObject viewerObject = KnoxicGameObject::createGameObject(knoxicDevice);
        viewerObject.transform.translation = {0.0f, 0.0f, -2.5f};
        viewerObject.transform.rotation = {0.0f, 0.0f, 0.0f};

        MouseMovementController cameraControllerMouse{};
        KeybordMovementController cameraControllerKeybord{cameraControllerMouse};

        // Initialize editor system
        editorSystem = std::make_unique<KnoxicEditorSystem>(
            knoxicWindow,
            knoxicDevice,
            knoxicRenderer,
            cameraControllerMouse,
            cameraControllerKeybord,
            renderableSystem,
            pointLightSystem,
            spotLightSystem,
            directionalLightSystem
        );

        auto currentTime = std::chrono::high_resolution_clock::now();
        VkExtent2D previousExtent = knoxicRenderer.getSwapChainExtent();

        cameraControllerMouse.init(knoxicWindow.getGLFWwindow());

        while(!knoxicWindow.shouldClose()) {
            glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            // Update editor system
            editorSystem->update(knoxicWindow.getGLFWwindow(), frameTime);

            // Only update camera controls if not in editor mode
            if (!editorSystem->isEditorMode()) {
                cameraControllerKeybord.moveInPlaneXZ(knoxicWindow.getGLFWwindow(), frameTime, viewerObject);
                cameraControllerMouse.updateLook(knoxicWindow.getGLFWwindow(), frameTime, viewerObject);
            }
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = knoxicRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(75.0f), aspect, 0.01f, 100.0f);
            
            if (auto commandBuffer = knoxicRenderer.beginFrame()) {
                // Check if window was resized and recreate post-processing resources
                VkExtent2D currentExtent = knoxicRenderer.getSwapChainExtent();
                if (currentExtent.width != previousExtent.width || currentExtent.height != previousExtent.height) {
                    postProcessSystem->recreate(currentExtent);
                    previousExtent = currentExtent;
                }

                int frameIndex = knoxicRenderer.getFrameIndex();
                FrameInfo frameInfo {
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex]
                };

                // Update materials
                materialSystem.updateMaterials(frameInfo, *materialSetLayout, *materialPool, renderableSystem);

                // Update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
                ubo.inverseView = camera.getInverseView();
                pointLightVkSystem.update(frameInfo, ubo);
                spotLightVkSystem.update(frameInfo, ubo);
                directionalLightVkSystem.update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                // Render scene to HDR buffer
                VkRenderPassBeginInfo hdrRenderPassInfo{};
                hdrRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                hdrRenderPassInfo.renderPass = postProcessSystem->getHDRRenderPass();
                hdrRenderPassInfo.framebuffer = postProcessSystem->getHDRFramebuffer(frameIndex);
                hdrRenderPassInfo.renderArea.offset = {0, 0};
                hdrRenderPassInfo.renderArea.extent = knoxicRenderer.getSwapChainExtent();

                std::array<VkClearValue, 2> clearValues{};
                clearValues[0].color = {{0.01f, 0.01f, 0.01f, 1.0f}};
                clearValues[1].depthStencil = {1.0f, 0};
                hdrRenderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                hdrRenderPassInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffer, &hdrRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                
                // Set viewport and scissor for HDR rendering
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(knoxicRenderer.getSwapChainExtent().width);
                viewport.height = static_cast<float>(knoxicRenderer.getSwapChainExtent().height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                VkRect2D scissor{{0, 0}, knoxicRenderer.getSwapChainExtent()};
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                
                renderSystem.renderGameObjects(frameInfo);
                pointLightVkSystem.render(frameInfo);
                spotLightVkSystem.render(frameInfo);
                directionalLightVkSystem.render(frameInfo);
                vkCmdEndRenderPass(commandBuffer);

                // Apply post-processing
                auto& postProcSettings = gCoordinator.GetComponent<PostProcessingComponent>(cameraEntity);
                postProcessSystem->renderPostProcess(commandBuffer, frameIndex, postProcSettings);

                // Render final composite to swapchain
                knoxicRenderer.beginSwapChainRenderPass(commandBuffer);
                postProcessSystem->renderFinalComposite(
                    commandBuffer,
                    knoxicRenderer.getSwapChainRenderPass(),
                    frameIndex,
                    postProcSettings
                );

                // Render ImGui
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                // Enable docking with transparent background
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
                const ImGuiViewport* imgui_viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(imgui_viewport->WorkPos);
                ImGui::SetNextWindowSize(imgui_viewport->WorkSize);
                ImGui::SetNextWindowViewport(imgui_viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
                
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent background
                ImGui::Begin("DockSpace", nullptr, window_flags);
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);
                
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
                ImGui::End();

                // Render editor UI if in editor mode
                if (editorSystem->isEditorMode()) {
                    editorSystem->renderUI();
                } else {
                    // Simple info window in play mode
                    ImGui::Begin("Game Info");
                    ImGui::Text("Press F11 to enter Editor Mode");
                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    ImGui::End();
                }

                ImGui::Render();
                ImDrawData* draw_data = ImGui::GetDrawData();
                ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

                knoxicRenderer.endSwapChainRenderPass(commandBuffer);
                knoxicRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(knoxicDevice.device());
    }

    void App::loadGameObjects() {
        std::shared_ptr<KnoxicModel> knoxicModel;

        // Creates a directional light entity
        // Entity dirLight1 = gCoordinator.CreateEntity();
        // TransformComponent dl1T{};
        // dl1T.translation = {0.0f, -3.0f, 0.0f};
        // dl1T.rotation = {glm::radians(30.0f), glm::radians(-90.0f), 0.0f}; // Direction of light
        // dl1T.scale = glm::vec3(0.1f);
        // gCoordinator.AddComponent(dirLight1, dl1T);
        // DirectionalLightComponent dl1C{};
        // dl1C.lightIntensity = 0.3f;
        // gCoordinator.AddComponent(dirLight1, dl1C);
        // gCoordinator.AddComponent(dirLight1, ColorComponent{glm::vec3{1.0f, 0.9f, 0.7f}});

        // -- First scene --
        {
            // Creates the bloom text entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/bloom_text.fbx");
            Entity bloomText = gCoordinator.CreateEntity();
            TransformComponent bloomTextTransform{};
            bloomTextTransform.translation = {-1.5f, -1.0f, 4.5f};
            bloomTextTransform.rotation = {glm::radians(180.0f), 0.0f, 0.0f};
            gCoordinator.AddComponent(bloomText, bloomTextTransform);
            gCoordinator.AddComponent(bloomText, ModelComponent{knoxicModel});
            MaterialComponent bloomTextMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            bloomTextMat.setRoughness(0.8f);
            bloomTextMat.setMetallic(0.7f);
            bloomTextMat.setColor(glm::vec3(0.0f, 0.0f, 0.1f));
            bloomTextMat.setEmission(glm::vec3(0.0f, 0.5f, 1.0f), 5.0f);
            gCoordinator.AddComponent(bloomText, bloomTextMat);

            // Creates the flat vase entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/flat_vase.obj");
            Entity flatVase = gCoordinator.CreateEntity();
            TransformComponent flatVaseTransform{};
            flatVaseTransform.translation = {-0.5f, 0.5f, 0.0f};
            flatVaseTransform.scale = {3.0f, 1.5f, 3.0f};
            gCoordinator.AddComponent(flatVase, flatVaseTransform);
            gCoordinator.AddComponent(flatVase, ModelComponent{knoxicModel});
            MaterialComponent flatVaseMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            flatVaseMat.setRoughness(0.8f);
            flatVaseMat.setMetallic(0.7f);
            gCoordinator.AddComponent(flatVase, flatVaseMat);

            // Creates the smooth vase entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/smooth_vase.obj");
            Entity smoothVase = gCoordinator.CreateEntity();
            TransformComponent smoothVaseTransform{};
            smoothVaseTransform.translation = {0.5f, 0.5f, 0.0f};
            smoothVaseTransform.scale = {3.0f, 1.5f, 3.0f};
            gCoordinator.AddComponent(smoothVase, smoothVaseTransform);
            gCoordinator.AddComponent(smoothVase, ModelComponent{knoxicModel});
            MaterialComponent smoothVaseMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            smoothVaseMat.setRoughness(0.8f);
            smoothVaseMat.setMetallic(0.7f);
            gCoordinator.AddComponent(smoothVase, smoothVaseMat);

            // Creates the floor entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            Entity floor = gCoordinator.CreateEntity();
            TransformComponent floorTransform{};
            floorTransform.translation = {0.0f, 0.5f, 0.0f};
            floorTransform.scale = {3.0f, 1.0f, 3.0f};
            gCoordinator.AddComponent(floor, floorTransform);
            gCoordinator.AddComponent(floor, ModelComponent{knoxicModel});
            MaterialComponent floorMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            floorMat.setMetallic(0.7f);
            floorMat.setRoughness(0.3f);
            gCoordinator.AddComponent(floor, floorMat);

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

                for (int i = 0; i < static_cast<int>(lightColors.size()); i++) {
                    Entity lightEnt = gCoordinator.CreateEntity();
                    TransformComponent t{};
                    t.scale = glm::vec3(0.05f);
                    auto rotateLight = glm::rotate(
                        glm::mat4(1.0f),
                        (i * glm::two_pi<float>()) / lightColors.size(),
                        glm::vec3{0.0f, -1.0f, 0.0f}
                    );
                    t.translation = glm::vec3(rotateLight * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f));
                    gCoordinator.AddComponent(lightEnt, t);
                    gCoordinator.AddComponent(lightEnt, PointLightComponent{0.5f});
                    gCoordinator.AddComponent(lightEnt, ColorComponent{lightColors[i]});
                }
            }
        }

        // -- Second scene --
        {
            // Creates the vase entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/smooth_vase.fbx");
            Entity vase = gCoordinator.CreateEntity();
            TransformComponent vaseTransform{};
            vaseTransform.translation = {10.0f, 0.5f, 0.0f};
            vaseTransform.scale = {3.0f, 1.5f, 3.0f};
            gCoordinator.AddComponent(vase, vaseTransform);
            gCoordinator.AddComponent(vase, ModelComponent{knoxicModel});
            MaterialComponent vaseMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            vaseMat.setRoughness(0.8f);
            vaseMat.setMetallic(0.7f);
            gCoordinator.AddComponent(vase, vaseMat);
            
            // Creates the floor entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            Entity floor2 = gCoordinator.CreateEntity();
            TransformComponent floor2Transform{};
            floor2Transform.translation = {10.0f, 0.5f, 0.0f};
            floor2Transform.scale = {3.0f, 1.0f, 3.0f};
            gCoordinator.AddComponent(floor2, floor2Transform);
            gCoordinator.AddComponent(floor2, ModelComponent{knoxicModel});
            MaterialComponent floor2Mat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            floor2Mat.loadAlbedoTexture("res/textures/missing.png");
            floor2Mat.setRoughness(0.5f);
            floor2Mat.setMetallic(0.0f);
            gCoordinator.AddComponent(floor2, floor2Mat);

            // Creates a point light entity
            Entity pointLight1 = gCoordinator.CreateEntity();
            TransformComponent pl1T{};
            pl1T.translation = {10.0f, -0.5f, -2.0f};
            pl1T.scale = glm::vec3(0.05f);
            gCoordinator.AddComponent(pointLight1, pl1T);
            gCoordinator.AddComponent(pointLight1, PointLightComponent{0.5f});
            gCoordinator.AddComponent(pointLight1, ColorComponent{glm::vec3{1.0f, 1.0f, 1.0f}});
        }

        // -- Third scene --
        {
            // Creates the medievalHelmet entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/sketchfab/medieval_helmet/scene.gltf");
            Entity medievalHelmet = gCoordinator.CreateEntity();
            TransformComponent helmetT{};
            helmetT.translation = {-10.0f, 0.5f, 0.0f};
            helmetT.scale = {0.03f, 0.03f, 0.03f};
            helmetT.rotation = {glm::radians(90.0f), 0.0f, 0.0f};
            gCoordinator.AddComponent(medievalHelmet, helmetT);
            gCoordinator.AddComponent(medievalHelmet, ModelComponent{knoxicModel});
            MaterialComponent helmetMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            helmetMat.loadAlbedoTexture("res/sketchfab/medieval_helmet/textures/medieval_helmet.jpeg");
            helmetMat.setRoughness(0.02f);
            helmetMat.setMetallic(2.0f);
            gCoordinator.AddComponent(medievalHelmet, helmetMat);

            // Creates the floor entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            Entity floor3 = gCoordinator.CreateEntity();
            TransformComponent floor3T{};
            floor3T.translation = {-10.0f, 0.5f, 0.0f};
            floor3T.scale = {3.0f, 1.0f, 3.0f};
            gCoordinator.AddComponent(floor3, floor3T);
            gCoordinator.AddComponent(floor3, ModelComponent{knoxicModel});
            MaterialComponent floor3Mat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            floor3Mat.loadAlbedoTexture("res/textures/woodPanels.jpg");
            floor3Mat.setRoughness(0.5f);
            floor3Mat.setMetallic(0.3f);
            gCoordinator.AddComponent(floor3, floor3Mat);

            // Creates the wall entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            Entity wall = gCoordinator.CreateEntity();
            TransformComponent wallT{};
            wallT.translation = {-10.0f, -2.5f, 3.0f};
            wallT.scale = {3.0f, 1.0f, 3.0f};
            wallT.rotation = {glm::radians(180.0f), glm::radians(90.0f), glm::radians(90.0f)};
            gCoordinator.AddComponent(wall, wallT);
            gCoordinator.AddComponent(wall, ModelComponent{knoxicModel});
            MaterialComponent wallMat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            wallMat.loadAlbedoTexture("res/textures/stoneSlate/castle_wall_slates_diff_4k.jpg");
            wallMat.loadNormalTexture("res/textures/stoneSlate/castle_wall_slates_nor_dx_4k.jpg");
            wallMat.setRoughness(0.002f);
            wallMat.setMetallic(3.0f);
            gCoordinator.AddComponent(wall, wallMat);

            // Creates the wall2 entity
            knoxicModel = KnoxicModel::createModelFromFile(knoxicDevice, "res/models/quad.obj");
            Entity wall2 = gCoordinator.CreateEntity();
            TransformComponent wall2T{};
            wall2T.translation = {-13.0f, -2.5f, 0.0f};
            wall2T.scale = {3.0f, 1.0f, 3.0f};
            wall2T.rotation = {0.0f, 0.0f, glm::radians(90.0f)};
            gCoordinator.AddComponent(wall2, wall2T);
            gCoordinator.AddComponent(wall2, ModelComponent{knoxicModel});
            MaterialComponent wall2Mat{std::make_shared<KnoxicMaterial>(knoxicDevice)};
            wall2Mat.loadAlbedoTexture("res/textures/stoneSlate/castle_wall_slates_diff_4k.jpg");
            wall2Mat.loadNormalTexture("res/textures/stoneSlate/castle_wall_slates_nor_dx_4k.jpg");
            wall2Mat.setRoughness(0.002f);
            wall2Mat.setMetallic(3.0f);
            gCoordinator.AddComponent(wall2, wall2Mat);

            // Creates a point light entity
            Entity pointLight2 = gCoordinator.CreateEntity();
            TransformComponent pl2T{};
            pl2T.translation = {-10.0f, -0.5f, -2.0f};
            pl2T.scale = glm::vec3(0.08f);
            gCoordinator.AddComponent(pointLight2, pl2T);
            gCoordinator.AddComponent(pointLight2, PointLightComponent{0.8f});
            gCoordinator.AddComponent(pointLight2, ColorComponent{glm::vec3{1.0f, 1.0f, 1.0f}});

            // Creates a spot light entity pointing at helmet
            Entity spotLight2 = gCoordinator.CreateEntity();
            TransformComponent sl2T{};
            sl2T.translation = {-12.0f, -2.0f, -2.0f};
            sl2T.rotation = {glm::radians(135.0f), glm::radians(45.0f), glm::radians(90.0f)};
            sl2T.scale = glm::vec3(0.08f);
            gCoordinator.AddComponent(spotLight2, sl2T);
            SpotLightComponent sl2C{};
            sl2C.lightIntensity = 3.0f;
            sl2C.innerCutoff = 15.0f;
            sl2C.outerCutoff = 25.0f;
            gCoordinator.AddComponent(spotLight2, sl2C);
            gCoordinator.AddComponent(spotLight2, ColorComponent{glm::vec3{1.0f, 0.5f, 0.0f}});
        }
    }
}