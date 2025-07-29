#pragma once

#include "knoxic_window.hpp"
#include "knoxic_device.hpp"
#include "knoxic_game_object.hpp"
#include "knoxic_renderer.hpp"
#include "knoxic_descriptors.hpp"

#include <vector>

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
            std::vector<KnoxicGameObject> gameObjects;
    };
}