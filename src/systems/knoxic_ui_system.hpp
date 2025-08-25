#pragma once

#include "../core/vulkan/knoxic_vk_device.hpp"
#include "../core/knoxic_window.hpp"
#include "../graphics/knoxic_frame_info.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <functional>
#include <unordered_map>
#include <vector>

namespace knoxic {

    class KnoxicUISystem {
    public:
        KnoxicUISystem(KnoxicWindow& window, KnoxicDevice& device, VkRenderPass renderPass, uint32_t imageCount);
        ~KnoxicUISystem();

        KnoxicUISystem(const KnoxicUISystem&) = delete;
        KnoxicUISystem& operator=(const KnoxicUISystem&) = delete;

        void newFrame();
        void render(VkCommandBuffer commandBuffer);
        void endFrame();

        // Editor state management
        void setEditorMode(bool enabled) { editorModeEnabled = enabled; }
        bool isEditorMode() const { return editorModeEnabled; }

        // UI creation helpers
        void showDemoWindow(bool* show = nullptr);
        void showSceneHierarchy(FrameInfo& frameInfo);
        void showInspector(FrameInfo& frameInfo);
        void showPerformanceWindow();
        void showAssetBrowser();
        void showConsole();

        // Custom window creation
        using WindowDrawFunction = std::function<void()>;
        void addCustomWindow(const std::string& name, WindowDrawFunction drawFunc);
        void removeCustomWindow(const std::string& name);

        // Style and theme management
        void setDarkTheme();
        void setLightTheme();
        void setCustomTheme();

        // Public access to custom windows for iteration
        const std::unordered_map<std::string, WindowDrawFunction>& getCustomWindows() const { return customWindows; }

    private:
        void setupImGui();
        void cleanupImGui();
        void setupDescriptorPool();

        // UI Window implementations
        void drawMainMenuBar();
        void drawSceneHierarchyWindow(FrameInfo& frameInfo);
        void drawInspectorWindow(FrameInfo& frameInfo);
        void drawPerformanceWindow();
        void drawAssetBrowserWindow();
        void drawConsoleWindow();

        KnoxicWindow& knoxicWindow;
        KnoxicDevice& knoxicDevice;
        VkRenderPass renderPass;

        VkDescriptorPool imguiPool;
        bool editorModeEnabled = false;
        
        // UI State
        bool showDemo = false;
        bool showSceneHierarchyWindow = true;
        bool showInspectorWindow = true;
        bool showPerformanceWindowFlag = false;
        bool showAssetBrowserWindow = false;
        bool showConsoleWindow = false;

        // Selected object for inspector
        KnoxicGameObject::id_t selectedObjectId = 0;
        bool hasSelectedObject = false;

        // Custom windows
        std::unordered_map<std::string, WindowDrawFunction> customWindows;

        // Performance tracking
        float frameTime = 0.0f;
        float fps = 0.0f;
        int frameCount = 0;
        float timeAccumulator = 0.0f;

        // Console messages
        struct ConsoleMessage {
            std::string message;
            ImVec4 color;
            float timestamp;
        };
        std::vector<ConsoleMessage> consoleMessages;
        static const size_t MAX_CONSOLE_MESSAGES = 100;

        // Asset browser state
        std::string currentAssetPath = "res/";
    };
}