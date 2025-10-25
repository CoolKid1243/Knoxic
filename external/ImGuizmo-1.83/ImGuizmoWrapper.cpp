// ImGuizmoWrapper.cpp
// Purpose: ensure ImGui math operators are defined and provide a shim for removed API
// so that ImGuizmo compiles against modern ImGui versions.

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

// Include imgui.h with math ops defined
#include "imgui.h"

// Provide a no-op shim for the deprecated function so ImGuizmo can call it.
// ImGui::CaptureMouseFromApp() used to exist; newer ImGui removed it.
// Defining this inline function keeps ABI stable for compile-time.
namespace ImGui {
    inline void CaptureMouseFromApp() { /* no-op shim for removed API */ }
}

// Now include ImGuizmo's implementation (compiles with above macro/shim)
#include "ImGuizmo.cpp"