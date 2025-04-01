// ImGui Bezier widget. @r-lyeh, public domain
// v1.03: improve grabbing, confine grabbers to area option, adaptive size, presets, preview.
// v1.02: add BezierValue(); comments; usage
// v1.01: out-of-bounds coord snapping; custom border width; spacing; cosmetics
// v1.00: initial version
//
// [ref] http://robnapier.net/faster-bezier
// [ref] http://easings.net/es#easeInSine
//
// Usage:
// {  static float v[5] = { 0.390f, 0.575f, 0.565f, 1.000f }; 
//    ImGui::Bezier( "easeOutSine", v );       // draw
//    float y = ImGui::BezierValue( 0.5f, v ); // x delta in [0..1] range
// }

#include "ImGuiCurveTest.h"
#include <vector>

namespace ImGui {
    template<int steps>
    void bezier_table(ImVec2 P[4], ImVec2 results[steps + 1]) {
        static float C[(steps + 1) * 4], * K = 0;
        if (!K) {
            K = C;
            for (unsigned step = 0; step <= steps; ++step) {
                float t = (float)step / (float)steps;
                C[step * 4 + 0] = (1 - t) * (1 - t) * (1 - t);   // * P0
                C[step * 4 + 1] = 3 * (1 - t) * (1 - t) * t; // * P1
                C[step * 4 + 2] = 3 * (1 - t) * t * t;     // * P2
                C[step * 4 + 3] = t * t * t;               // * P3
            }
        }
        for (unsigned step = 0; step <= steps; ++step) {
            ImVec2 point = {
                K[step * 4 + 0] * P[0].x + K[step * 4 + 1] * P[1].x + K[step * 4 + 2] * P[2].x + K[step * 4 + 3] * P[3].x,
                K[step * 4 + 0] * P[0].y + K[step * 4 + 1] * P[1].y + K[step * 4 + 2] * P[2].y + K[step * 4 + 3] * P[3].y
            };
            results[step] = point;
        }
    }

    float BezierValue(float dt01, float P[4]) {
        enum { STEPS = 256 };
        ImVec2 Q[4] = { { 0, 0 }, { P[0], P[1] }, { P[2], P[3] }, { 1, 1 } };
        ImVec2 results[STEPS + 1];
        bezier_table<STEPS>(Q, results);
        return results[(int)((dt01 < 0 ? 0 : dt01 > 1 ? 1 : dt01) * STEPS)].y;
    }

    int Bezier(const char* label, float P[5]) {
        // visuals
        enum { SMOOTHNESS = 64 }; // curve smoothness: the higher number of segments, the smoother curve
        enum { CURVE_WIDTH = 4 }; // main curved line width
        enum { LINE_WIDTH = 1 }; // handlers: small lines width
        enum { GRAB_RADIUS = 8 }; // handlers: circle radius
        enum { GRAB_BORDER = 2 }; // handlers: circle border width
        enum { AREA_CONSTRAINED = true }; // should grabbers be constrained to grid area?
        enum { AREA_WIDTH = 128 }; // area width in pixels. 0 for adaptive size (will use max avail width)

        // bezier widget
        const ImGuiStyle& Style = GetStyle();
        const ImGuiIO& IO = GetIO();
        ImDrawList* DrawList = GetWindowDrawList();
        ImGuiWindow* Window = GetCurrentWindow();
        if (Window->SkipItems)
            return false;

        // header and spacing
        int changed = SliderFloat4(label, P, 0, 1, "%.3f", 1.0f);
        int hovered = IsItemActive() || IsItemHovered(); // IsItemDragged() ?
        Dummy(ImVec2(0, 3));

        // prepare canvas
        const float avail = GetContentRegionAvail().x;
        const float dim = AREA_WIDTH > 0 ? AREA_WIDTH : avail;
        ImVec2 Canvas(dim, dim);

        ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(bb);
        if (!ItemAdd(bb, NULL))
            return changed;

        const ImGuiID id = Window->GetID(label);
        hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id, NULL);

        RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

        // background grid
        for (int i = 0; i <= Canvas.x; i += (Canvas.x / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x + i, bb.Min.y),
                ImVec2(bb.Min.x + i, bb.Max.y),
                GetColorU32(ImGuiCol_TextDisabled));
        }
        for (int i = 0; i <= Canvas.y; i += (Canvas.y / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x, bb.Min.y + i),
                ImVec2(bb.Max.x, bb.Min.y + i),
                GetColorU32(ImGuiCol_TextDisabled));
        }

        // eval curve
        ImVec2 Q[4] = { { 0, 0 }, { P[0], P[1] }, { P[2], P[3] }, { 1, 1 } };
        ImVec2 results[SMOOTHNESS + 1];
        bezier_table<SMOOTHNESS>(Q, results);

        // control points: 2 lines and 2 circles
        {
            // handle grabbers
            ImVec2 mouse = GetIO().MousePos, pos[2];
            float distance[2];

            for (int i = 0; i < 2; ++i) {
                pos[i] = ImVec2(P[i * 2 + 0], 1 - P[i * 2 + 1]) * (bb.Max - bb.Min) + bb.Min;
                distance[i] = (pos[i].x - mouse.x) * (pos[i].x - mouse.x) + (pos[i].y - mouse.y) * (pos[i].y - mouse.y);
            }

            int selected = distance[0] < distance[1] ? 0 : 1;
            if (distance[selected] < (4 * GRAB_RADIUS * 4 * GRAB_RADIUS)) {
                SetTooltip("(%4.3f, %4.3f)", P[selected * 2 + 0], P[selected * 2 + 1]);

                if (/*hovered &&*/ (IsMouseClicked(0) || IsMouseDragging(0))) {
                    float& px = (P[selected * 2 + 0] += GetIO().MouseDelta.x / Canvas.x);
                    float& py = (P[selected * 2 + 1] -= GetIO().MouseDelta.y / Canvas.y);

                    if (AREA_CONSTRAINED) {
                        px = (px < 0 ? 0 : (px > 1 ? 1 : px));
                        py = (py < 0 ? 0 : (py > 1 ? 1 : py));
                    }

                    changed = true;
                }
            }
        }

        // if (hovered || changed) DrawList->PushClipRectFullScreen();

        // draw curve
        {
            ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
            for (int i = 0; i < SMOOTHNESS; ++i) {
                ImVec2 p = { results[i + 0].x, 1 - results[i + 0].y };
                ImVec2 q = { results[i + 1].x, 1 - results[i + 1].y };
                ImVec2 r(p.x * (bb.Max.x - bb.Min.x) + bb.Min.x, p.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
                ImVec2 s(q.x * (bb.Max.x - bb.Min.x) + bb.Min.x, q.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
                DrawList->AddLine(r, s, color, CURVE_WIDTH);
            }
        }


        ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);

        // draw lines and grabbers
        float luma = IsItemActive() || IsItemHovered() ? 0.5f : 1.0f;
        ImVec4 pink(1.00f, 0.00f, 0.75f, luma), cyan(0.00f, 0.75f, 1.00f, luma);
        ImVec2 p1 = ImVec2(P[0], 1 - P[1]) * (bb.Max - bb.Min) + bb.Min;
        ImVec2 p2 = ImVec2(P[2], 1 - P[3]) * (bb.Max - bb.Min) + bb.Min;
        DrawList->AddLine(ImVec2(bb.Min.x, bb.Max.y), p1, ImColor(white), LINE_WIDTH);
        DrawList->AddLine(ImVec2(bb.Max.x, bb.Min.y), p2, ImColor(white), LINE_WIDTH);
        DrawList->AddCircleFilled(p1, GRAB_RADIUS, ImColor(white));
        DrawList->AddCircleFilled(p1, GRAB_RADIUS - GRAB_BORDER, ImColor(pink));
        DrawList->AddCircleFilled(p2, GRAB_RADIUS, ImColor(white));
        DrawList->AddCircleFilled(p2, GRAB_RADIUS - GRAB_BORDER, ImColor(cyan));

        // if (hovered || changed) DrawList->PopClipRect();

        return changed;
    }

    // simple linear interpolation between two points
    void lerp(point& dest, const point& a, const point& b, const float t) {
        dest.x = a.x + (b.x - a.x) * t;
        dest.y = a.y + (b.y - a.y) * t;
    }

    float EvaluateCurve(point points[4], const float t) {
        point ab, bc, cd, abbc, bccd, dest;
        lerp(ab, points[0], points[1], t);
        lerp(bc, points[1], points[2], t);
        lerp(cd, points[2], points[3], t);
        lerp(abbc, ab, bc, t);
        lerp(bccd, bc, cd, t);
        lerp(dest, abbc, bccd, t);
        return dest.y;
    }

    int DrawCurve(const char* label, point points[4]) {
        // visuals
        enum { SMOOTHNESS = 64 }; // curve smoothness: the higher number of segments, the smoother curve
        enum { CURVE_WIDTH = 4 }; // main curved line width
        enum { LINE_WIDTH = 1 }; // handlers: small lines width
        enum { GRAB_RADIUS = 8 }; // handlers: circle radius
        enum { GRAB_BORDER = 2 }; // handlers: circle border width
        enum { AREA_CONSTRAINED = true }; // should grabbers be constrained to grid area?
        enum { AREA_WIDTH = 128 }; // area width in pixels. 0 for adaptive size (will use max avail width)

        // bezier widget
        const ImGuiStyle& Style = GetStyle();
        const ImGuiIO& IO = GetIO();
        ImDrawList* DrawList = GetWindowDrawList();
        ImGuiWindow* Window = GetCurrentWindow();
        if (Window->SkipItems)
            return false;

        // header and spacing
        int changed = 0;
        int hovered = IsItemActive() || IsItemHovered(); // IsItemDragged() ?
        Dummy(ImVec2(0, 3));

        // prepare canvas
        const float avail = GetContentRegionAvail().x;
        const float dim = AREA_WIDTH > 0 ? AREA_WIDTH : avail;
        ImVec2 Canvas(dim, dim);

        ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(bb);
        if (!ItemAdd(bb, NULL))
            return changed;

        const ImGuiID id = Window->GetID(label);
        hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id, NULL);

        RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

        // background grid
        for (int i = 0; i <= Canvas.x; i += (Canvas.x / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x + i, bb.Min.y),
                ImVec2(bb.Min.x + i, bb.Max.y),
                GetColorU32(ImGuiCol_TextDisabled));
        }
        for (int i = 0; i <= Canvas.y; i += (Canvas.y / 4)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x, bb.Min.y + i),
                ImVec2(bb.Max.x, bb.Min.y + i),
                GetColorU32(ImGuiCol_TextDisabled));
        }

        // eval curve
        ImVec2 results[SMOOTHNESS + 1];
        for (int i = 0; i <= SMOOTHNESS; ++i) {
            float t = (float)i / (float)SMOOTHNESS;
            results[i].x = t;
            results[i].y = EvaluateCurve(points, t);
        }

        // control points: 2 lines and 2 circles
        {
            // handle grabbers
            ImVec2 mouse = GetIO().MousePos, pos[4];
            float distance[4];

            for (int i = 0; i < 4; ++i) {
                pos[i] = ImVec2(points[i].x, 1 - points[i].y) * (bb.Max - bb.Min) + bb.Min;
                distance[i] = (pos[i].x - mouse.x) * (pos[i].x - mouse.x) + (pos[i].y - mouse.y) * (pos[i].y - mouse.y);
            }

            for (int i = 0; i < 4; ++i) {
                if (distance[i] < (4 * GRAB_RADIUS * 4 * GRAB_RADIUS)) {
                    SetTooltip("(%4.3f, %4.3f)", points[i].x, points[i].y);

                    if (/*hovered &&*/ (IsMouseClicked(0) || IsMouseDragging(0))) {
                        float& px = (points[i].x += GetIO().MouseDelta.x / Canvas.x);
                        float& py = (points[i].y -= GetIO().MouseDelta.y / Canvas.y);

                        if (AREA_CONSTRAINED) {
                            px = (px < 0 ? 0 : (px > 1 ? 1 : px));
                            py = (py < 0 ? 0 : (py > 1 ? 1 : py));
                        }

                        changed = true;
                    }
                }
            }
        }

        // draw curve
        {
            ImColor color(GetStyle().Colors[ImGuiCol_PlotLines]);
            for (int i = 0; i < SMOOTHNESS; ++i) {
                ImVec2 p = { results[i + 0].x, 1 - results[i + 0].y };
                ImVec2 q = { results[i + 1].x, 1 - results[i + 1].y };
                ImVec2 r(p.x * (bb.Max.x - bb.Min.x) + bb.Min.x, p.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
                ImVec2 s(q.x * (bb.Max.x - bb.Min.x) + bb.Min.x, q.y * (bb.Max.y - bb.Min.y) + bb.Min.y);
                DrawList->AddLine(r, s, color, CURVE_WIDTH);
            }
        }

        ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);

        // draw lines and grabbers
        float luma = IsItemActive() || IsItemHovered() ? 0.5f : 1.0f;
        ImVec4 pink(1.00f, 0.00f, 0.75f, luma), cyan(0.00f, 0.75f, 1.00f, luma);
        for (int i = 0; i < 4; ++i) {
            ImVec2 p = ImVec2(points[i].x, 1 - points[i].y) * (bb.Max - bb.Min) + bb.Min;
            DrawList->AddCircleFilled(p, GRAB_RADIUS, ImColor(white));
            DrawList->AddCircleFilled(p, GRAB_RADIUS - GRAB_BORDER, ImColor(i % 2 == 0 ? pink : cyan));
        }

        return changed;
    }
}