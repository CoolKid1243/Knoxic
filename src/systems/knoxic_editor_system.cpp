#include "knoxic_editor_system.hpp"

#include "../graphics/vulkan/knoxic_vk_material.hpp"
#include "../camera/knoxic_camera.hpp"

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <cstring>
#include <map>

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

    void KnoxicEditorSystem::setupDefaultStyle() {
        if (mStyleInitialized) return;
        
        ImGuiStyle& style = ImGui::GetStyle();
        
        // Dark theme colors
        style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
        style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

        // Spacing and sizing
        style.WindowPadding = ImVec2(4.0f, 4.0f);
        style.FramePadding = ImVec2(4.0f, 3.0f);
        style.ItemSpacing = ImVec2(4.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 21.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 10.0f;
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 2.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 2.0f;
        style.LogSliderDeadzone = 4.0f;
        style.TabRounding = 0.0f;
        
        mStyleInitialized = true;
    }

    void KnoxicEditorSystem::renderMenuBar() {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    // TODO: Implement new scene
                }
                if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                    // TODO: Implement open scene
                }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                    // TODO: Implement save scene
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    // TODO: Handle exit
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                    // TODO: Implement undo
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                    // TODO: Implement redo
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Preferences", "Ctrl+,")) {
                    // TODO: Open preferences
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("GameObject")) {
                if (ImGui::MenuItem("Create Empty")) {
                    // TODO: Create empty entity
                }
                if (ImGui::MenuItem("3D Object", nullptr, false, false)) {
                    // TODO: Create 3D object submenu
                }
                if (ImGui::MenuItem("Light", nullptr, false, false)) {
                    // TODO: Create light submenu
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Window")) {
                if (ImGui::MenuItem("Hierarchy", nullptr, &mShowHierarchy)) {}
                if (ImGui::MenuItem("Scene", nullptr, &mShowScene)) {}
                if (ImGui::MenuItem("Inspector", nullptr, &mShowInspector)) {}
                if (ImGui::MenuItem("Project", nullptr, &mShowProject)) {}
                if (ImGui::MenuItem("Console", nullptr, &mShowConsole)) {}
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    // TODO: Show about dialog
                }
                ImGui::EndMenu();
            }
            
            ImGui::EndMenuBar();
        }
    }

    void KnoxicEditorSystem::renderToolbar() {
        // Use full height of the toolbar window
        ImGuiStyle& style = ImGui::GetStyle();
        float buttonWidth = 80.0f;
        float spacing = 4.0f;
        
        // Add padding for better button visibility
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
        
        // Play/Pause/Stop buttons
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
        if (ImGui::Button("Play", ImVec2(buttonWidth, -1))) {
            // TODO: Implement play
        }
        ImGui::PopStyleColor(3);
        
        ImGui::SameLine(0, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.4f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.5f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.3f, 0.1f, 1.0f));
        if (ImGui::Button("Pause", ImVec2(buttonWidth, -1))) {
            // TODO: Implement pause
        }
        ImGui::PopStyleColor(3);
        
        ImGui::SameLine(0, spacing);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button("Stop", ImVec2(buttonWidth, -1))) {
            // TODO: Implement stop
        }
        ImGui::PopStyleColor(3);
        
        ImGui::SameLine(0, spacing * 3);
        
        // Gizmo operation buttons
        ImGui::Text("Gizmo:");
        ImGui::SameLine(0, spacing);
        if (ImGui::RadioButton("Move", mGizmoOperation == 0)) mGizmoOperation = 0;
        ImGui::SameLine(0, spacing);
        if (ImGui::RadioButton("Rotate", mGizmoOperation == 1)) mGizmoOperation = 1;
        ImGui::SameLine(0, spacing);
        if (ImGui::RadioButton("Scale", mGizmoOperation == 2)) mGizmoOperation = 2;
        
        ImGui::PopStyleVar();
    }

    std::string KnoxicEditorSystem::getEntityDisplayName(Entity entity) {
        std::stringstream ss;
        
        // Try to determine entity type based on components
        if (gCoordinator.HasComponent<PointLightComponent>(entity)) {
            ss << "Point Light";
        } else if (gCoordinator.HasComponent<SpotLightComponent>(entity)) {
            ss << "Spot Light";
        } else if (gCoordinator.HasComponent<DirectionalLightComponent>(entity)) {
            ss << "Directional Light";
        } else if (gCoordinator.HasComponent<ModelComponent>(entity)) {
            ss << "GameObject";
        } else {
            ss << "Entity " << entity;
        }
        
        return ss.str();
    }

    void KnoxicEditorSystem::renderHierarchyWindow() {
        if (!mRenderableSystem) {
            ImGui::Text("No renderable system found");
            return;
        }

        // Search bar
        static char searchBuffer[256] = "";
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search...", searchBuffer, sizeof(searchBuffer));
        ImGui::PopItemWidth();
        ImGui::Separator();

        // Display entities in a tree-like structure
        ImGui::BeginChild("HierarchyContent", ImVec2(0, 0), false);
        
        // Collect all entities
        std::vector<Entity> allEntities;
        
        // Add renderable entities
        for (Entity entity : mRenderableSystem->mEntities) {
            allEntities.push_back(entity);
        }
        
        // Add light entities that aren't already in the list
        if (mPointLightSystem) {
            for (Entity entity : mPointLightSystem->mEntities) {
                bool found = false;
                for (Entity existing : allEntities) {
                    if (existing == entity) {
                        found = true;
                        break;
                    }
                }
                if (!found) allEntities.push_back(entity);
            }
        }
        
        if (mSpotLightSystem) {
            for (Entity entity : mSpotLightSystem->mEntities) {
                bool found = false;
                for (Entity existing : allEntities) {
                    if (existing == entity) {
                        found = true;
                        break;
                    }
                }
                if (!found) allEntities.push_back(entity);
            }
        }
        
        if (mDirectionalLightSystem) {
            for (Entity entity : mDirectionalLightSystem->mEntities) {
                bool found = false;
                for (Entity existing : allEntities) {
                    if (existing == entity) {
                        found = true;
                        break;
                    }
                }
                if (!found) allEntities.push_back(entity);
            }
        }

        // Count occurrences of each entity type
        std::map<std::string, int> nameCounts;
        for (Entity entity : allEntities) {
            std::string baseName = getEntityDisplayName(entity);
            nameCounts[baseName]++;
        }
        
        // Track how many of each type we've displayed so far
        std::map<std::string, int> displayedCounts;
        
        // Display entities with numbering
        for (Entity entity : allEntities) {
            std::string baseName = getEntityDisplayName(entity);
            std::string entityName = baseName;
            
            // If there are multiple entities with the same name add a number
            if (nameCounts[baseName] > 1) {
                int count = ++displayedCounts[baseName];
                entityName = baseName + " (" + std::to_string(count) + ")";
            }
            
            // Filter by search
            if (searchBuffer[0] != '\0' && entityName.find(searchBuffer) == std::string::npos) {
                continue;
            }
            
            bool isSelected = (mSelectedEntity == entity);
            
            // Use Selectable for entity items
            if (ImGui::Selectable(entityName.c_str(), isSelected)) {
                mSelectedEntity = entity;
            }
        }
        
        ImGui::EndChild();
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

        // Scene view toolbar
        ImGui::BeginChild("SceneToolbar", ImVec2(0, 25), false, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("Scene");
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Text("2D");
        ImGui::SameLine();
        if (ImGui::Button("Shaded")) {
            // TODO: Change view mode
        }
        ImGui::SameLine();
        if (ImGui::Button("...")) {
            // TODO: View options menu
        }
        ImGui::EndChild();
        
        ImGui::Separator();

        // Display placeholder for now - the actual rendering will be handled in app.cpp
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
        if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Draw a placeholder rectangle with a dark background
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 30, 30, 255));
        drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(50, 50, 50, 255), 0.0f, 0, 1.0f);

        // Display info overlay
        ImGui::SetCursorPos(ImVec2(10, 35));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 0.8f));
        ImGui::Text("Size: %.0f x %.0f", canvasSize.x, canvasSize.y);
        ImGui::PopStyleColor();
    }
    
    void KnoxicEditorSystem::renderSceneWindowWithGizmo(const KnoxicCamera& camera) {
        // Calculate the actual content area (excluding title bar)
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        ImVec2 windowPos = ImGui::GetWindowPos();
        
        mSceneWindowPos.x = windowPos.x + contentMin.x;
        mSceneWindowPos.y = windowPos.y + contentMin.y;
        mSceneWindowSize.x = contentMax.x - contentMin.x;
        mSceneWindowSize.y = contentMax.y - contentMin.y;
        mSceneWindowFocused = ImGui::IsWindowFocused();

        // Scene view toolbar
        ImGui::BeginChild("SceneToolbar", ImVec2(0, 25), false, ImGuiWindowFlags_NoScrollbar);
        ImGui::Text("Scene");
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::Text("2D");
        ImGui::SameLine();
        if (ImGui::Button("Shaded")) {
            // TODO: Change view mode
        }
        ImGui::SameLine();
        if (ImGui::Button("...")) {
            // TODO: View options menu
        }
        ImGui::EndChild();
        
        ImGui::Separator();

        // Display placeholder for now - the actual rendering will be handled in app.cpp
        ImVec2 canvasSize = ImGui::GetContentRegionAvail();
        if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
        if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        // Draw a placeholder rectangle with a dark background
        drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 30, 30, 255));
        drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(50, 50, 50, 255), 0.0f, 0, 1.0f);

        // Setup ImGuizmo - must be called after ImGui::NewFrame()
        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(drawList);
        ImGuizmo::SetRect(mSceneWindowPos.x, mSceneWindowPos.y, mSceneWindowSize.x, mSceneWindowSize.y);

        // Render gizmo for selected entity if it has a transform
        if (mSelectedEntity != 0 && gCoordinator.HasComponent<TransformComponent>(mSelectedEntity)) {
            auto& transform = gCoordinator.GetComponent<TransformComponent>(mSelectedEntity);
            
            // Build transform matrix
            glm::mat4 transformMatrix = transform.mat4();
            
            // Get camera matrices
            glm::mat4 viewMatrix = camera.getView();
            glm::mat4 projectionMatrix = camera.getProjection();
            
            // Convert to float arrays for ImGuizmo (column-major)
            float matrix[16];
            float view[16];
            float projection[16];
            
            // GLM matrices are column-major, ImGuizmo expects column-major
            const float* transformPtr = glm::value_ptr(transformMatrix);
            const float* viewPtr = glm::value_ptr(viewMatrix);
            const float* projPtr = glm::value_ptr(projectionMatrix);
            
            std::memcpy(matrix, transformPtr, sizeof(float) * 16);
            std::memcpy(view, viewPtr, sizeof(float) * 16);
            std::memcpy(projection, projPtr, sizeof(float) * 16);
            
            // Determine gizmo operation
            ImGuizmo::OPERATION gizmoOp = ImGuizmo::TRANSLATE;
            if (mGizmoOperation == 1) gizmoOp = ImGuizmo::ROTATE;
            else if (mGizmoOperation == 2) gizmoOp = ImGuizmo::SCALE;
            
            // Manipulate the matrix
            ImGuizmo::Manipulate(
                view,
                projection,
                gizmoOp,
                ImGuizmo::LOCAL,
                matrix,
                nullptr,
                nullptr
            );
            
            // If gizmo was used, update the transform
            if (ImGuizmo::IsUsing()) {
                // Decompose matrix back to transform
                float translation[3], rotation[3], scale[3];
                ImGuizmo::DecomposeMatrixToComponents(matrix, translation, rotation, scale);
                
                transform.translation = glm::vec3(translation[0], translation[1], translation[2]);
                transform.rotation = glm::vec3(rotation[0], rotation[1], rotation[2]);
                transform.scale = glm::vec3(scale[0], scale[1], scale[2]);
            }
        }

        // Display info overlay
        ImGui::SetCursorPos(ImVec2(10, 35));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 0.8f));
        ImGui::Text("Size: %.0f x %.0f", canvasSize.x, canvasSize.y);
        ImGui::PopStyleColor();
    }

    void KnoxicEditorSystem::renderInspectorWindow() {
        if (mSelectedEntity == 0) {
            ImGui::TextWrapped("No entity selected");
            ImGui::Spacing();
            ImGui::TextWrapped("Select an entity from the Hierarchy to view its components.");
            return;
        }

        // Entity header
        std::string entityName = getEntityDisplayName(mSelectedEntity);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        if (ImGui::CollapsingHeader(entityName.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap)) {
            ImGui::PopStyleColor(3);
            
            // Display Transform Component
            if (gCoordinator.HasComponent<TransformComponent>(mSelectedEntity)) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto& transform = gCoordinator.GetComponent<TransformComponent>(mSelectedEntity);
                    
                    ImGui::PushItemWidth(-1);
                    ImGui::Text("Position");
                    float translation[3] = {transform.translation.x, transform.translation.y, transform.translation.z};
                    if (ImGui::DragFloat3("##Position", translation, 0.1f)) {
                        transform.translation = {translation[0], translation[1], translation[2]};
                    }

                    ImGui::Text("Rotation");
                    float rotation[3] = {transform.rotation.x, transform.rotation.y, transform.rotation.z};
                    if (ImGui::DragFloat3("##Rotation", rotation, 0.01f)) {
                        transform.rotation = {rotation[0], rotation[1], rotation[2]};
                    }

                    ImGui::Text("Scale");
                    float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
                    if (ImGui::DragFloat3("##Scale", scale, 0.1f)) {
                        transform.scale = {scale[0], scale[1], scale[2]};
                    }
                    ImGui::PopItemWidth();
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
                    auto& materialComp = gCoordinator.GetComponent<MaterialComponent>(mSelectedEntity);
                    KnoxicMaterial* mat = materialComp.material.get();

                    if (!mat) {
                        ImGui::Text("No Material Assigned");
                    } else {
                        auto& props = mat->getProperties();

                        // Spacing
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Albedo");

                        // Albedo color
                        ImGui::PushID("AlbedoColor");
                        float col[3] = { props.albedo.r, props.albedo.g, props.albedo.b };
                        if (ImGui::ColorEdit3("Color", col)) {
                            mat->setAlbedo({ col[0], col[1], col[2] });
                        }
                        ImGui::PopID();

                        // Texture loading
                        if (ImGui::Button("Load Albedo Texture...")) {
                            // TODO: open texture browser
                        }
                        ImGui::SameLine();
                        if (mat->hasTextures()) {
                            ImGui::Text("(Texture loaded)");
                        }

                        // Drag & drop for texture
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH")) {
                                const char* droppedPath = (const char*)payload->Data;
                                mat->loadAlbedoTexture(droppedPath);
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        ImGui::Text("Metallic");

                        float metallic = props.metallic;
                        float smoothness = 1.0f - props.roughness; // convert roughness -> smoothness

                        ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f);
                        ImGui::SliderFloat("Smoothness", &smoothness, 0.0f, 1.0f);

                        mat->setMetallic(metallic);
                        mat->setRoughness(1.0f - smoothness);

                        // Metallic map loading
                        if (ImGui::Button("Load Metallic Map...")) {
                            // TODO: open texture browser
                        }

                        // Drag & drop for metallic map
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH")) {
                                mat->loadMetallicMap((const char*)payload->Data);
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        ImGui::Text("Normal Map");

                        // Normal map loading
                        if (ImGui::Button("Load Normal Map...")) {
                            // TODO: open texture browser
                        }

                        // Drag & drop for normal map
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FILE_PATH")) {
                                mat->loadNormalTexture((const char*)payload->Data);
                            }
                            ImGui::EndDragDropTarget();
                        }

                        ImGui::Spacing();
                        ImGui::Separator();

                        // Use a persistent state for emission checkbox per entity
                        static std::map<Entity, bool> emissionStates;
                        bool& emissionEnabled = emissionStates[mSelectedEntity];
                        
                        // Initialize from material if not set
                        if (emissionStates.find(mSelectedEntity) == emissionStates.end()) {
                            emissionEnabled = props.emissionStrength > 0.01f;
                        }

                        if (ImGui::Checkbox("Emission", &emissionEnabled)) {
                            if (!emissionEnabled) {
                                mat->setEmissionStrength(0.0f);
                            } else if (props.emissionStrength < 0.01f) {
                                // If enabling and strength is 0, set a default value
                                mat->setEmissionStrength(1.0f);
                            }
                        }
                        
                        if (emissionEnabled) {
                            ImGui::PushID("EmissionColor");
                            float eColor[3] = { props.emissionColor.r, props.emissionColor.g, props.emissionColor.b };
                            if (ImGui::ColorEdit3("Color", eColor)) {
                                mat->setEmissionColor({eColor[0],eColor[1],eColor[2]});
                            }
                            ImGui::PopID();

                            float strength = props.emissionStrength;
                            if (ImGui::SliderFloat("Intensity", &strength, 0.0f, 50.0f)) {
                                mat->setEmissionStrength(strength);
                                // Update state if user sets strength to 0
                                if (strength < 0.01f) {
                                    emissionEnabled = false;
                                }
                            }
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                    }
                }
            }

            // Display Model Component
            if (gCoordinator.HasComponent<ModelComponent>(mSelectedEntity)) {
                if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Text("Model Component");
                    ImGui::Text("(Model info display not yet implemented)");
                }
            }
        } else {
            ImGui::PopStyleColor(3);
        }
    }

    void KnoxicEditorSystem::renderProjectWindow() {
        ImGui::Text("Project");
        ImGui::Separator();
        ImGui::Text("Assets");
        ImGui::Text("(Project browser not yet implemented)");
    }

    void KnoxicEditorSystem::renderConsoleWindow() {
        ImGui::Text("Console");
        ImGui::Separator();
        ImGui::Text("(Console output not yet implemented)");
    }

    void KnoxicEditorSystem::renderUI(const KnoxicCamera& camera) {
        if (!mEditorMode) {
            return; // Don't render editor UI in play mode
        }

        // Setup style on first render
        if (!mStyleInitialized) {
            setupDefaultStyle();
        }

        // Get the main viewport - check for validity
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        if (!viewport) {
            return; // Safety check
        }
        
        // Ensure valid sizes
        float viewportWidth = viewport->WorkSize.x > 0 ? viewport->WorkSize.x : 1920.0f;
        float viewportHeight = viewport->WorkSize.y > 0 ? viewport->WorkSize.y : 1080.0f;
        
        // Menu bar (docked to top)
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewportWidth, 0));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        if (ImGui::Begin("##MainMenuBar", nullptr, 
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus)) {
            renderMenuBar();
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // Toolbar (below menu bar)
        float menuBarHeight = ImGui::GetFrameHeight();
        float toolbarHeight = 40.0f; // Increased from 30 to 40
        float toolbarY = viewport->WorkPos.y + menuBarHeight;
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, toolbarY));
        ImGui::SetNextWindowSize(ImVec2(viewportWidth, toolbarHeight));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        if (ImGui::Begin("##Toolbar", nullptr, 
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar)) {
            renderToolbar();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        // Calculate available space for docked windows (below menu bar and toolbar)
        float topOffset = menuBarHeight + toolbarHeight;
        float availableY = viewport->WorkPos.y + topOffset;
        float availableHeight = viewportHeight - topOffset;
        
        // Ensure positive sizes
        if (availableHeight < 100.0f) {
            availableHeight = 100.0f;
        }

        // Hierarchy window (left side) - dockable
        if (mShowHierarchy) {
            ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, availableY), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(250, availableHeight), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Hierarchy", &mShowHierarchy)) {
                renderHierarchyWindow();
            }
            ImGui::End();
        }

        // Inspector window (right side) - dockable
        if (mShowInspector) {
            float inspectorX = viewport->WorkPos.x + viewportWidth - 300;
            ImGui::SetNextWindowPos(ImVec2(inspectorX, availableY), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(300, availableHeight), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Inspector", &mShowInspector)) {
                renderInspectorWindow();
            }
            ImGui::End();
        }

        // Scene window (center) - dockable, main viewport
        if (mShowScene) {
            float sceneX = viewport->WorkPos.x + (mShowHierarchy ? 250.0f : 0.0f);
            float sceneWidth = viewportWidth - (mShowHierarchy ? 250.0f : 0.0f) - (mShowInspector ? 300.0f : 0.0f);
            if (sceneWidth < 100.0f) sceneWidth = 100.0f;
            
            ImGui::SetNextWindowPos(ImVec2(sceneX, availableY), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(sceneWidth, availableHeight), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Scene", &mShowScene)) {
                renderSceneWindowWithGizmo(camera);
            }
            ImGui::End();
        }

        // Project window (bottom) - dockable
        if (mShowProject) {
            float projectX = viewport->WorkPos.x + (mShowHierarchy ? 250.0f : 0.0f);
            float projectWidth = viewportWidth - (mShowHierarchy ? 250.0f : 0.0f) - (mShowInspector ? 300.0f : 0.0f);
            if (projectWidth < 100.0f) projectWidth = 100.0f;
            float projectY = availableY + availableHeight - 200.0f;
            
            ImGui::SetNextWindowPos(ImVec2(projectX, projectY), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(projectWidth, 200), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Project", &mShowProject)) {
                renderProjectWindow();
            }
            ImGui::End();
        }

        // Console window (bottom) - dockable
        if (mShowConsole) {
            float consoleY = availableY + availableHeight - 150.0f;
            ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, consoleY), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(viewportWidth, 150), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Console", &mShowConsole)) {
                renderConsoleWindow();
            }
            ImGui::End();
        }
    }
}