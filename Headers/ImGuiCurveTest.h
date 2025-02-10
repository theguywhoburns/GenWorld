#ifndef IMGUI_CURVE_TEST_H
#define IMGUI_CURVE_TEST_H


#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <time.h>

namespace ImGui {
    struct point {
        float x;
        float y;
    };

    float BezierValue(float dt01, float P[4]);
    int Bezier(const char* label, float P[5]);

    int DrawCurve(const char* label, point points[4]);
    void lerp(point& dest, const point& a, const point& b, const float t);
    float EvaluateCurve(point points[4], const float t);
}

#endif // IMGUI_CURVE_TEST_H