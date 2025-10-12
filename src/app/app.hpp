#pragma once

#include "../core/knoxic_window.hpp"
#include "../core/vulkan/knoxic_vk_device.hpp"
#include "../graphics/vulkan/knoxic_vk_renderer.hpp"
#include "../core/vulkan/knoxic_vk_descriptors.hpp"
#include "../core/ecs/ecs_systems.hpp"

#include <memory>

namespace knoxic {

    class App {
    public:
        static constexpr int WIDTH = 1152;
        static constexpr int HEIGHT = 758;

        App();
        ~App();

        App(const App &) = delete;
        App &operator=(const App &) = delete;

        void run();

    private:
        void loadGameObjects();

        Entity cameraEntity;

        KnoxicWindow knoxicWindow{WIDTH, HEIGHT, "Knoxic"};
        KnoxicDevice knoxicDevice{knoxicWindow};
        KnoxicRenderer knoxicRenderer{knoxicWindow, knoxicDevice};

        // Order of declarations matters
        std::unique_ptr<KnoxicDescriptorPool> globalPool{};
        std::unique_ptr<KnoxicDescriptorPool> materialPool{};

        // ECS systems
        std::shared_ptr<RenderableSystem> renderableSystem;
        std::shared_ptr<PointLightECSSystem> pointLightSystem;
    };
}