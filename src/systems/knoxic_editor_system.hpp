#pragma once

#include "../core/knoxic_window.hpp"
#include "../core/vulkan/knoxic_vk_device.hpp"
#include "../graphics/vulkan/knoxic_vk_renderer.hpp"
#include "../core/ecs/coordinator_instance.hpp"
#include "../core/ecs/components.hpp"
#include "../core/ecs/ecs_systems.hpp"
#include "../camera/knoxic_camera.hpp"
#include "../input/mouse_movement_controller.hpp"
#include "../input/keybord_movement_controller.hpp"

#include <imgui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>
#include <GLFW/glfw3.h>

namespace knoxic {

    class KnoxicEditorSystem {
    public:
        KnoxicEditorSystem(
            KnoxicWindow& window,
            KnoxicDevice& device,
            KnoxicRenderer& renderer,
            MouseMovementController& mouseController,
            KeybordMovementController& keyboardController,
            std::shared_ptr<RenderableSystem> renderableSystem,
            std::shared_ptr<PointLightECSSystem> pointLightSystem,
            std::shared_ptr<SpotLightECSSystem> spotLightSystem,
            std::shared_ptr<DirectionalLightECSSystem> directionalLightSystem
        );

        void update(GLFWwindow* glfwWindow, float frameTime);
        void renderUI(const KnoxicCamera& camera);
        bool isEditorMode() const { return mEditorMode; }
        Entity getSelectedEntity() const { return mSelectedEntity; }
        void setSelectedEntity(Entity entity) { mSelectedEntity = entity; }
        ImVec2 getSceneWindowSize() const { return mSceneWindowSize; }
        ImVec2 getSceneWindowPos() const { return mSceneWindowPos; }
        bool isSceneWindowFocused() const { return mSceneWindowFocused; }

    private:
        void handleInput(GLFWwindow* glfwWindow);
        void setupDefaultStyle();
        void renderMenuBar();
        void renderToolbar();
        void renderHierarchyWindow();
        void renderSceneWindow();
        void renderSceneWindowWithGizmo(const KnoxicCamera& camera);
        void renderInspectorWindow();
        void renderProjectWindow();
        void renderConsoleWindow();
        std::string getEntityDisplayName(Entity entity);

        KnoxicWindow& mWindow;
        KnoxicDevice& mDevice;
        KnoxicRenderer& mRenderer;
        MouseMovementController& mMouseController;
        KeybordMovementController& mKeyboardController;

        std::shared_ptr<RenderableSystem> mRenderableSystem;
        std::shared_ptr<PointLightECSSystem> mPointLightSystem;
        std::shared_ptr<SpotLightECSSystem> mSpotLightSystem;
        std::shared_ptr<DirectionalLightECSSystem> mDirectionalLightSystem;

        bool mEditorMode = false;
        Entity mSelectedEntity = 0;
        bool mShowHierarchy = true;
        bool mShowScene = true;
        bool mShowInspector = true;
        bool mShowProject = true;
        bool mShowConsole = false;
        bool mStyleInitialized = false;

        // Scene window rendering
        ImVec2 mSceneWindowSize{0, 0};
        ImVec2 mSceneWindowPos{0, 0};
        bool mSceneWindowFocused = false;
        
        // Gizmo state
        int mGizmoOperation = 0; // 0=Translate, 1=Rotate, 2=Scale
    };
}