#ifndef IMGUI_CURVE_TEST_H
#define IMGUI_CURVE_TEST_H


#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <time.h>

namespace ImGui {
    float BezierValue(float dt01, float P[4]);
    int Bezier(const char* label, float P[5]);
}

#endif // IMGUI_CURVE_TEST_H