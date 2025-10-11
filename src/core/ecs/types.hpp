#pragma once

#include <cstdint>
#include <bitset>

using Entity = std::uint32_t;
using ComponentType = std::uint8_t;

constexpr std::size_t MAX_ENTITIES = 5000;
constexpr std::size_t MAX_COMPONENTS = 32;

using Signature = std::bitset<MAX_COMPONENTS>;