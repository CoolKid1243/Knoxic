#pragma once

#include "../core/knoxic_window.hpp"
#include "../core/knoxic_device.hpp"
#include "../object/knoxic_game_object.hpp"
#include "../graphics/knoxic_renderer.hpp"
#include "../core/knoxic_descriptors.hpp"

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

            KnoxicWindow knoxicWindow{WIDTH, HEIGHT, "Knoxic"};
            KnoxicDevice knoxicDevice{knoxicWindow};
            KnoxicRenderer knoxicRenderer{knoxicWindow, knoxicDevice};

            // Order of declarations matters
            std::unique_ptr<KnoxicDescriptorPool> globalPool{};
            KnoxicGameObject::Map gameObjects;
    };
}