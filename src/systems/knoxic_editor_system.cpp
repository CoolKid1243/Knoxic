#include "knoxic_editor_system.hpp"
#include "vulkan/knoxic_vk_post_process_system.hpp"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>
#include <sstream>
#include <iomanip>

namespace knoxic {

    KnoxicEditorSystem::KnoxicEditorSystem(
        KnoxicWindow& window,
        KnoxicDevice& device,
        KnoxicRenderer& renderer,
        MouseMovementController& mouseController,
        KeybordMovementController& keyboardController,
        std::shared_ptr<RenderableSystem> renderableSystem,
        std::shared_ptr<PointLightECSSystem> pointLightSystem,
        std::shared_ptr<SpotLightECSSystem> spotLightSystem,
        std::shared_ptr<DirectionalLightECSSystem> directionalLightSystem
    ) : mWindow(window),
        mDevice(device),
        mRenderer(renderer),
        mMouseController(mouseController),
        mKeyboardController(keyboardController),
        mRenderableSystem(renderableSystem),
        mPointLightSystem(pointLightSystem),
        mSpotLightSystem(spotLightSystem),
        mDirectionalLightSystem(directionalLightSystem)
    {
    }

    void KnoxicEditorSystem::update(GLFWwindow* glfwWindow, float frameTime) {
        handleInput(glfwWindow);
    }

    void KnoxicEditorSystem::handleInput(GLFWwindow* glfwWindow) {
        // Toggle editor mode with f10
        static bool f10PressedLastFrame = false;
        if (glfwGetKey(glfwWindow, GLFW_KEY_F10) == GLFW_PRESS) {
            if (!f10PressedLastFrame) {
                mEditorMode = !mEditorMode;
                
                // Update mouse visibility based on editor mode
                if (mEditorMode) {
                    // Editor mode: show mouse cursor
                    mKeyboardController.mouseHidden = false;
                    mMouseController.mouseHidden = false;
                    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    mMouseController.resetCursor(glfwWindow);
                } else {
                    // Play mode: hide mouse cursor
                    mKeyboardController.mouseHidden = true;
                    mMouseController.mouseHidden = true;
                    glfwSetInputMode(glfwWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    mMouseController.resetCursor(glfwWindow);
                }
            }
            f10PressedLastFrame = true;
        } else {
            f10PressedLastFrame = false;
        }
    }

    void KnoxicEditorSystem::renderUI() {
        if (!mEditorMode) {
            return; // Don't render editor UI in play mode
        }

        // Render Hierarchy window
        if (mShowHierarchy) {
            ImGui::Begin("Hierarchy", &mShowHierarchy);
            renderHierarchyWindow();
            ImGui::End();
        }

        // Render Scene window
        if (mShowScene) {
            ImGui::Begin("Scene", &mShowScene);
            renderSceneWindow();
            ImGui::End();
        }

        // Render Inspector window
        if (mShowInspector) {
            ImGui::Begin("Inspector", &mShowInspector);
            renderInspectorWindow();
            ImGui::End();
        }
    }

    void KnoxicEditorSystem::renderHierarchyWindow() {
        if (!mRenderableSystem) {
            ImGui::Text("No renderable system found");
            return;
        }

        // Display entities
        ImGui::Text("Entities:");
        ImGui::Separator();

        // Iterate through entities in the renderable system
        for (Entity entity : mRenderableSystem->mEntities) {
            std::stringstream ss;
            ss << "Entity " << entity;
            
            // Try to determine entity type based on components
            std::string entityName = ss.str();
            if (gCoordinator.HasComponent<PointLightComponent>(entity)) {
                entityName += " (Point Light)";
            } else if (gCoordinator.HasComponent<SpotLightComponent>(entity)) {
                entityName += " (Spot Light)";
            } else if (gCoordinator.HasComponent<DirectionalLightComponent>(entity)) {
                entityName += " (Directional Light)";
            } else if (gCoordinator.HasComponent<ModelComponent>(entity)) {
                entityName += " (Model)";
            }

            // Make it selectable
            bool isSelected = (mSelectedEntity == entity);
            if (ImGui::Selectable(entityName.c_str(), isSelected)) {
                mSelectedEntity = entity;
            }
        }

        // Also show other entities (lights, etc.)
        if (mPointLightSystem) {
            for (Entity entity : mPointLightSystem->mEntities) {
                if (gCoordinator.HasComponent<ModelComponent>(entity)) {
                    continue; // Already shown above
                }
                std::stringstream ss;
                ss << "Entity " << entity << " (Point Light)";
                bool isSelected = (mSelectedEntity == entity);
                if (ImGui::Selectable(ss.str().c_str(), isSelected)) {
                    mSelectedEntity = entity;
                }
            }
        }

        if (mSpotLightSystem) {
            for (Entity entity : mSpotLightSystem->mEntities) {
                if (gCoordinator.HasComponent<ModelComponent>(entity)) {
                    continue; // Already shown above
                }
                std::stringstream ss;
                ss << "Entity " << entity << " (Spot Light)";
                bool isSelected = (mSelectedEntity == entity);
                if (ImGui::Selectable(ss.str().c_str(), isSelected)) {
                    mSelectedEntity = entity;
                }
            }
        }

        if (mDirectionalLightSystem) {
            for (Entity entity : mDirectionalLightSystem->mEntities) {
                if (gCoordinator.HasComponent<ModelComponent>(entity)) {
                    continue; // Already shown above
                }
                std::stringstream ss;
                ss << "Entity " << entity << " (Directional Light)";
                bool isSelected = (mSelectedEntity == entity);
                if (ImGui::Selectable(ss.str().c_str(), isSelected)) {
                    mSelectedEntity = entity;
                }
            }
        }
    }

    void KnoxicEditorSystem::renderSceneWindow() {
        // Calculate the actual content area (excluding title bar)
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        ImVec2 windowPos = ImGui::GetWindowPos();
        
        mSceneWindowPos.x = windowPos.x + contentMin.x;
        mSceneWindowPos.y = windowPos.y + contentMin.y;
        mSceneWindowSize.x = contentMax.x - contentMin.x;
        mSceneWindowSize.y = contentMax.y - contentMin.y;
        mSceneWindowFocused = ImGui::IsWindowFocused();

        // Display placeholder for now - the actual rendering will be handled in app.cpp
        // We'll use ImGui::Image() to display the rendered texture
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
        if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Draw a placeholder rectangle
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(20, 20, 20, 255));
        drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(100, 100, 100, 255));

        // Display info
        ImGui::SetCursorPos(ImVec2(10, 30));
        ImGui::Text("Size: %.0f x %.0f", canvasSize.x, canvasSize.y);
    }

    void KnoxicEditorSystem::renderInspectorWindow() {
        if (mSelectedEntity == 0) {
            ImGui::Text("No entity selected");
            ImGui::Text("Select an entity from the Hierarchy");
            return;
        }

        ImGui::Text("Entity: %u", mSelectedEntity);
        ImGui::Separator();

        // Display Transform Component
        if (gCoordinator.HasComponent<TransformComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& transform = gCoordinator.GetComponent<TransformComponent>(mSelectedEntity);
                
                ImGui::Text("Translation:");
                float translation[3] = {transform.translation.x, transform.translation.y, transform.translation.z};
                if (ImGui::DragFloat3("##Translation", translation, 0.1f)) {
                    transform.translation = {translation[0], translation[1], translation[2]};
                }

                ImGui::Text("Rotation:");
                float rotation[3] = {transform.rotation.x, transform.rotation.y, transform.rotation.z};
                if (ImGui::DragFloat3("##Rotation", rotation, 0.01f)) {
                    transform.rotation = {rotation[0], rotation[1], rotation[2]};
                }

                ImGui::Text("Scale:");
                float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
                if (ImGui::DragFloat3("##Scale", scale, 0.1f)) {
                    transform.scale = {scale[0], scale[1], scale[2]};
                }
            }
        }

        // Display Color Component
        if (gCoordinator.HasComponent<ColorComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& color = gCoordinator.GetComponent<ColorComponent>(mSelectedEntity);
                float colorValues[3] = {color.color.r, color.color.g, color.color.b};
                if (ImGui::ColorEdit3("Color", colorValues)) {
                    color.color = {colorValues[0], colorValues[1], colorValues[2]};
                }
            }
        }

        // Display Point Light Component
        if (gCoordinator.HasComponent<PointLightComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& light = gCoordinator.GetComponent<PointLightComponent>(mSelectedEntity);
                ImGui::DragFloat("Intensity", &light.lightIntensity, 0.1f, 0.0f, 10.0f);
            }
        }

        // Display Spot Light Component
        if (gCoordinator.HasComponent<SpotLightComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& light = gCoordinator.GetComponent<SpotLightComponent>(mSelectedEntity);
                ImGui::DragFloat("Intensity", &light.lightIntensity, 0.1f, 0.0f, 10.0f);
                ImGui::DragFloat("Inner Cutoff", &light.innerCutoff, 0.1f, 0.0f, 90.0f);
                ImGui::DragFloat("Outer Cutoff", &light.outerCutoff, 0.1f, 0.0f, 90.0f);
            }
        }

        // Display Directional Light Component
        if (gCoordinator.HasComponent<DirectionalLightComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& light = gCoordinator.GetComponent<DirectionalLightComponent>(mSelectedEntity);
                ImGui::DragFloat("Intensity", &light.lightIntensity, 0.1f, 0.0f, 10.0f);
            }
        }

        // Display Material Component
        if (gCoordinator.HasComponent<MaterialComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& material = gCoordinator.GetComponent<MaterialComponent>(mSelectedEntity);
                ImGui::Text("Material Component");
                ImGui::Text("(Material properties editing not yet implemented)");
            }
        }

        // Display Model Component
        if (gCoordinator.HasComponent<ModelComponent>(mSelectedEntity)) {
            if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Model Component");
                ImGui::Text("(Model info display not yet implemented)");
            }
        }
    }
}