#include "app/app.hpp"

#include <cstdlib>
#include <iostream>
#include <exception>

int main() {
    knoxic::App app{};

    try {
        app.run();
    } catch(const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}