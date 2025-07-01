#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <time.h>
#include <vector>

namespace ImGui {
    struct point {
        float x;
        float y;
    };

    struct CurvePoint {
        point main;      // Main curve point
        point tangent;   // Tangent direction and magnitude (relative to main point)

        CurvePoint(float x = 0.0f, float y = 0.5f) {
            main = { x, y };
            tangent = { 0.15f, 0.0f };
        }
    };

    int DrawCurve(const char* label, std::vector<CurvePoint>& curvePoints);

    void lerp(point& dest, const point& a, const point& b, const float t);
    float EvaluateCurve(std::vector<CurvePoint>& curvePoints, const float t);
    float EvaluateHermiteSegment(const CurvePoint& p1, const CurvePoint& p2, float t);
    float Distance(const point& a, const point& b);
    int FindInsertPosition(std::vector<CurvePoint>& curvePoints, float clickX);
}