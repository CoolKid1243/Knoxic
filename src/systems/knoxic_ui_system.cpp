#include "knoxic_ui_system.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <chrono>

namespace knoxic {

    KnoxicUISystem::KnoxicUISystem(KnoxicWindow& window, KnoxicDevice& device, VkRenderPass renderPass, uint32_t imageCount) 
        : knoxicWindow{window}, knoxicDevice{device}, renderPass{renderPass} {
        setupDescriptorPool();
        setupImGui();
        setDarkTheme();
        
        // Add initial console message
        consoleMessages.push_back({"ImGui UI System initialized", ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 0.0f});
    }

    KnoxicUISystem::~KnoxicUISystem() {
        cleanupImGui();
        if (imguiPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(knoxicDevice.device(), imguiPool, nullptr);
        }
    }

    void KnoxicUISystem::setupDescriptorPool() {
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000;
        poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(poolSizes[0]));
        poolInfo.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(knoxicDevice.device(), &poolInfo, nullptr, &imguiPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ImGui descriptor pool!");
        }
    }

    void KnoxicUISystem::setupImGui() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Note: Docking may not be available in all ImGui versions
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        
        // Initialize ImGui for GLFW and Vulkan
        ImGui_ImplGlfw_InitForVulkan(knoxicWindow.getGLFWwindow(), true);
        
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = knoxicDevice.getInstance();
        initInfo.PhysicalDevice = knoxicDevice.getPhysicalDevice();
        initInfo.Device = knoxicDevice.device();
        initInfo.QueueFamily = knoxicDevice.findPhysicalQueueFamilies().graphicsFamily;
        initInfo.Queue = knoxicDevice.graphicsQueue();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = imguiPool;
        initInfo.Allocator = nullptr;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = 2;
        initInfo.CheckVkResultFn = nullptr;

        ImGui_ImplVulkan_Init(&initInfo);

        // Upload fonts
        VkCommandBuffer commandBuffer = knoxicDevice.beginSingleTimeCommands();
        // ImGui_ImplVulkan_CreateFontsTexture(); -> Error: Use of undeclared identifier 'ImGui_ImplVulkan_CreateFontsTexture'
        knoxicDevice.endSingleTimeCommands(commandBuffer);
        // ImGui_ImplVulkan_DestroyFontUploadObjects(); -> Error: Use of undeclared identifier 'ImGui_ImplVulkan_DestroyFontUploadObjects'
    }

    void KnoxicUISystem::cleanupImGui() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void KnoxicUISystem::newFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (editorModeEnabled) {
            // Simple windowed approach instead of docking
            drawMainMenuBar();
        }
    }

    void KnoxicUISystem::render(VkCommandBuffer commandBuffer) {
        if (editorModeEnabled) {
            ImGui::Render();
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }
    }

    void KnoxicUISystem::endFrame() {
        // Update performance metrics
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        frameTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        frameCount++;
        timeAccumulator += frameTime;
        if (timeAccumulator >= 1.0f) {
            fps = frameCount / timeAccumulator;
            frameCount = 0;
            timeAccumulator = 0.0f;
        }
    }

    void KnoxicUISystem::drawMainMenuBar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene")) {
                    consoleMessages.push_back({"New Scene created", ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                        static_cast<float>(ImGui::GetTime())});
                }
                if (ImGui::MenuItem("Open Scene")) {
                    consoleMessages.push_back({"Open Scene dialog", ImVec4(0.0f, 1.0f, 1.0f, 1.0f), 
                        static_cast<float>(ImGui::GetTime())});
                }
                if (ImGui::MenuItem("Save Scene")) {
                    consoleMessages.push_back({"Scene saved", ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 
                        static_cast<float>(ImGui::GetTime())});
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(knoxicWindow.getGLFWwindow(), true);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Scene Hierarchy", nullptr, &showSceneHierarchyWindow);
                ImGui::MenuItem("Inspector", nullptr, &showInspectorWindow);
                ImGui::MenuItem("Performance", nullptr, &showPerformanceWindowFlag);
                ImGui::MenuItem("Asset Browser", nullptr, &showAssetBrowserWindow);
                ImGui::MenuItem("Console", nullptr, &showConsoleWindow);
                ImGui::Separator();
                ImGui::MenuItem("Demo Window", nullptr, &showDemo);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::BeginMenu("Theme")) {
                    if (ImGui::MenuItem("Dark Theme")) setDarkTheme();
                    if (ImGui::MenuItem("Light Theme")) setLightTheme();
                    if (ImGui::MenuItem("Custom Theme")) setCustomTheme();
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void KnoxicUISystem::showDemoWindow(bool* show) {
        if (show ? *show : showDemo) {
            ImGui::ShowDemoWindow(show ? show : &showDemo);
        }
    }

    void KnoxicUISystem::showSceneHierarchy(FrameInfo& frameInfo) {
        if (editorModeEnabled && showSceneHierarchyWindow) {
            drawSceneHierarchyWindow(frameInfo);
        }
    }

    void KnoxicUISystem::showInspector(FrameInfo& frameInfo) {
        if (editorModeEnabled && showInspectorWindow) {
            drawInspectorWindow(frameInfo);
        }
    }

    void KnoxicUISystem::showPerformanceWindow() {
        if (editorModeEnabled && showPerformanceWindowFlag) {
            drawPerformanceWindow();
        }
    }

    void KnoxicUISystem::showAssetBrowser() {
        if (editorModeEnabled && showAssetBrowserWindow) {
            drawAssetBrowserWindow();
        }
    }

    void KnoxicUISystem::showConsole() {
        if (editorModeEnabled && showConsoleWindow) {
            drawConsoleWindow();
        }
    }

    void KnoxicUISystem::drawSceneHierarchyWindow(FrameInfo& frameInfo) {
        if (ImGui::Begin("Scene Hierarchy", &showSceneHierarchyWindow)) {
            if (ImGui::TreeNode("Scene Objects")) {
                for (auto& pair : frameInfo.gameObjects) {
                    auto& id = pair.first;
                    auto& gameObject = pair.second;
                    
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                    if (hasSelectedObject && selectedObjectId == id) {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }

                    std::string objName = "Object " + std::to_string(id);
                    if (gameObject.pointLight) objName += " (Light)";
                    if (gameObject.model) objName += " (Model)";

                    ImGui::TreeNodeEx(objName.c_str(), flags);
                    
                    if (ImGui::IsItemClicked()) {
                        selectedObjectId = id;
                        hasSelectedObject = true;
                    }
                }
                ImGui::TreePop();
            }
        }
        ImGui::End();
    }

    void KnoxicUISystem::drawInspectorWindow(FrameInfo& frameInfo) {
        if (ImGui::Begin("Inspector", &showInspectorWindow)) {
            if (hasSelectedObject && frameInfo.gameObjects.count(selectedObjectId) > 0) {
                auto& obj = frameInfo.gameObjects.at(selectedObjectId);
                
                ImGui::Text("Object ID: %u", selectedObjectId);
                ImGui::Separator();

                // Transform component
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Position", &obj.transform.translation[0], 0.1f);
                    ImGui::DragFloat3("Rotation", &obj.transform.rotation[0], 0.01f);
                    ImGui::DragFloat3("Scale", &obj.transform.scale[0], 0.1f, 0.1f);
                }

                // Color
                if (ImGui::CollapsingHeader("Appearance")) {
                    ImGui::ColorEdit3("Color", &obj.color[0]);
                }

                // Material component
                if (obj.material && obj.material->material) {
                    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto& matProps = const_cast<MaterialProperties&>(obj.material->material->getProperties());
                        
                        ImGui::ColorEdit3("Albedo", &matProps.albedo[0]);
                        ImGui::SliderFloat("Metallic", &matProps.metallic, 0.0f, 1.0f);
                        ImGui::SliderFloat("Roughness", &matProps.roughness, 0.0f, 1.0f);
                        ImGui::SliderFloat("AO", &matProps.ao, 0.0f, 1.0f);
                        ImGui::DragFloat2("Texture Offset", &matProps.textureOffset[0], 0.01f);
                        ImGui::DragFloat2("Texture Scale", &matProps.textureScale[0], 0.01f, 0.1f, 10.0f);
                    }
                }

                // Point light component
                if (obj.pointLight) {
                    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::SliderFloat("Intensity", &obj.pointLight->lightIntensity, 0.0f, 100.0f);
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Delete Object")) {
                    frameInfo.gameObjects.erase(selectedObjectId);
                    hasSelectedObject = false;
                    consoleMessages.push_back({"Object deleted", ImVec4(1.0f, 0.5f, 0.0f, 1.0f), 
                        static_cast<float>(ImGui::GetTime())});
                }
            } else {
                ImGui::Text("No object selected");
                ImGui::Text("Select an object from the Scene Hierarchy");
            }
        }
        ImGui::End();
    }

    void KnoxicUISystem::drawPerformanceWindow() {
        if (ImGui::Begin("Performance", &showPerformanceWindowFlag)) {
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", frameTime * 1000.0f, fps);
            ImGui::Text("Frame Time: %.3f ms", frameTime * 1000.0f);
            
            ImGui::Separator();
            ImGui::Text("Vulkan Info:");
            ImGui::Text("Device: %s", knoxicDevice.properties.deviceName);
            ImGui::Text("API Version: %u.%u.%u", 
                VK_VERSION_MAJOR(knoxicDevice.properties.apiVersion),
                VK_VERSION_MINOR(knoxicDevice.properties.apiVersion),
                VK_VERSION_PATCH(knoxicDevice.properties.apiVersion));

            ImGui::Separator();
            ImGui::Text("Memory Usage:");
            // Add memory usage stats here in the future
            ImGui::Text("GPU Memory: N/A");
            ImGui::Text("CPU Memory: N/A");
        }
        ImGui::End();
    }

    void KnoxicUISystem::drawAssetBrowserWindow() {
        if (ImGui::Begin("Asset Browser", &showAssetBrowserWindow)) {
            ImGui::Text("Current Path: %s", currentAssetPath.c_str());
            ImGui::Separator();
            
            // Simple file listing - replace with actual filesystem code when available
            ImGui::Text("[DIR] models");
            ImGui::Text("[DIR] textures");
            ImGui::Text("[DIR] shaders");
            ImGui::Text("example.obj");
            ImGui::Text("texture.png");
        }
        ImGui::End();
    }

    void KnoxicUISystem::drawConsoleWindow() {
        if (ImGui::Begin("Console", &showConsoleWindow)) {
            if (ImGui::Button("Clear")) {
                consoleMessages.clear();
            }
            
            ImGui::Separator();
            
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            
            for (const auto& message : consoleMessages) {
                ImGui::TextColored(message.color, "[%.1fs] %s", message.timestamp, message.message.c_str());
            }
            
            // Auto-scroll to bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
            
            ImGui::EndChild();
        }
        ImGui::End();
    }

    void KnoxicUISystem::addCustomWindow(const std::string& name, WindowDrawFunction drawFunc) {
        customWindows[name] = drawFunc;
    }

    void KnoxicUISystem::removeCustomWindow(const std::string& name) {
        customWindows.erase(name);
    }

    void KnoxicUISystem::setDarkTheme() {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.TabRounding = 0.0f;
    }

    void KnoxicUISystem::setLightTheme() {
        ImGui::StyleColorsLight();
    }

    void KnoxicUISystem::setCustomTheme() {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.3f;
        style.FrameRounding = 2.3f;
        style.ScrollbarRounding = 0;
        
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
        colors[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.27f, 0.27f, 0.54f, 0.83f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.32f, 0.32f, 0.63f, 0.87f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
    }
}