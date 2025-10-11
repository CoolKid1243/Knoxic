#pragma once

#include "system.hpp"

// Minimal ECS system types used to query entities for rendering and lighting
struct RenderableSystem : public System {};
struct PointLightECSSystem : public System {};