#include "HermiteCurve.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace ImGui {
    // Simple linear interpolation between two points
    void lerp(point& dest, const point& a, const point& b, const float t) {
        dest.x = a.x + (b.x - a.x) * t;
        dest.y = a.y + (b.y - a.y) * t;
    }

    // Calculate distance between two points
    float Distance(const point& a, const point& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return sqrt(dx * dx + dy * dy);
    }

    // Evaluate Hermite spline between two curve points
    float EvaluateHermiteSegment(const CurvePoint& p1, const CurvePoint& p2, float t) {
        float y1 = p1.main.y;
        float y2 = p2.main.y;
        float dx = p2.main.x - p1.main.x;

        // Convert tangent to actual slope (dy/dx)
        float m1 = p1.tangent.y / (p1.tangent.x + 0.001f); // Prevent division by zero
        float m2 = p2.tangent.y / (p2.tangent.x + 0.001f);

        // Scale tangents by the x-distance between points
        float t1 = m1 * dx;
        float t2 = m2 * dx;

        // Hermite basis functions
        float t_sq = t * t;
        float t_cu = t_sq * t;

        float h1 = 2 * t_cu - 3 * t_sq + 1;
        float h2 = -2 * t_cu + 3 * t_sq;
        float h3 = t_cu - 2 * t_sq + t;
        float h4 = t_cu - t_sq;

        return h1 * y1 + h2 * y2 + h3 * t1 + h4 * t2;
    }

    // Evaluate the entire curve at parameter t (0 to 1 across all segments)
    float EvaluateCurve(std::vector<CurvePoint>& curvePoints, float t) {
        if (curvePoints.size() < 2) return 0.0f;

        t = ImClamp(t, 0.0f, 1.0f);

        // Before first point: return 0
        if (t < curvePoints[0].main.x) {
            return 0.0f;
        }

        // After last point: return 1
        if (t > curvePoints.back().main.x) {
            return 1.0f;
        }

        // Find which segment we're in based on x-values
        for (int i = 0; i < curvePoints.size() - 1; i++) {
            float x1 = curvePoints[i].main.x;
            float x2 = curvePoints[i + 1].main.x;

            if (t >= x1 && t <= x2) {
                if (x2 - x1 < 0.001f) return curvePoints[i].main.y; // Avoid division by zero

                float segmentT = (t - x1) / (x2 - x1);
                return EvaluateHermiteSegment(curvePoints[i], curvePoints[i + 1], segmentT);
            }
        }

        // Fallback (shouldn't reach here)
        return curvePoints.back().main.y;
    }

    // Find the best position to insert a new point
    int FindInsertPosition(std::vector<CurvePoint>& curvePoints, float clickX) {
        for (int i = 0; i < curvePoints.size() - 1; i++) {
            if (clickX >= curvePoints[i].main.x && clickX <= curvePoints[i + 1].main.x) {
                return i + 1;
            }
        }
        // Add at the end or beginning based on position
        if (clickX < curvePoints[0].main.x) return 0;
        return curvePoints.size();
    }

    int DrawCurve(const char* label, std::vector<CurvePoint>& curvePoints) {
        // Ensure minimum 2 points
        if (curvePoints.size() < 2) {
            curvePoints.clear();
            curvePoints.push_back(CurvePoint(0.0f, 0.0f));
            curvePoints.push_back(CurvePoint(1.0f, 1.0f));
        }

        // Sort points by x coordinate to maintain order
        std::sort(curvePoints.begin(), curvePoints.end(),
            [](const CurvePoint& a, const CurvePoint& b) {
                return a.main.x < b.main.x;
            });

        // Visual constants
        enum { SMOOTHNESS = 256 };
        enum { CURVE_WIDTH = 4 };
        enum { LINE_WIDTH = 2 };
        enum { GRAB_RADIUS = 8 };
        enum { TANGENT_RADIUS = 5 };
        enum { GRAB_BORDER = 2 };
        enum { AREA_WIDTH = 256 };

        const float TANGENT_LENGTH = 0.15f; // Constant length for tangent handles

        const ImGuiStyle& Style = GetStyle();
        const ImGuiIO& IO = GetIO();
        ImDrawList* DrawList = GetWindowDrawList();
        ImGuiWindow* Window = GetCurrentWindow();
        if (Window->SkipItems)
            return false;

        int changed = 0;
        int hovered = IsItemActive() || IsItemHovered();
        Dummy(ImVec2(0, 3));

        // Prepare canvas
        const float avail = GetContentRegionAvail().x;
        const float dim = AREA_WIDTH > 0 ? AREA_WIDTH : avail;
        ImVec2 Canvas(dim, dim);

        ImRect bb(Window->DC.CursorPos, Window->DC.CursorPos + Canvas);
        ItemSize(bb);
        if (!ItemAdd(bb, 0))
            return changed;

        const ImGuiID id = Window->GetID(label);
        hovered |= 0 != ItemHoverable(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id, 0);

        RenderFrame(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg, 1), true, Style.FrameRounding);

        // Background grid
        for (int i = 0; i <= Canvas.x; i += (Canvas.x / 8)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x + i, bb.Min.y),
                ImVec2(bb.Min.x + i, bb.Max.y),
                GetColorU32(ImGuiCol_TextDisabled, 0.3f));
        }
        for (int i = 0; i <= Canvas.y; i += (Canvas.y / 8)) {
            DrawList->AddLine(
                ImVec2(bb.Min.x, bb.Min.y + i),
                ImVec2(bb.Max.x, bb.Min.y + i),
                GetColorU32(ImGuiCol_TextDisabled, 0.3f));
        }

        ImVec2 mouse = GetIO().MousePos;

        // Handle point addition (double-click)
        static double lastClickTime = 0.0;
        static ImVec2 lastClickPos;
        if (hovered && IsMouseClicked(0)) {
            double currentTime = GetTime();
            if (currentTime - lastClickTime < 0.3 && Distance({ mouse.x, mouse.y }, { lastClickPos.x, lastClickPos.y }) < 10) {
                // Double click detected
                ImVec2 relativePos = (mouse - bb.Min) / (bb.Max - bb.Min);
                relativePos.y = 1.0f - relativePos.y; // Flip Y coordinate

                // Clamp to valid range
                relativePos.x = ImClamp(relativePos.x, 0.0f, 1.0f);
                relativePos.y = ImClamp(relativePos.y, 0.0f, 1.0f);

                int insertPos = FindInsertPosition(curvePoints, relativePos.x);
                CurvePoint newPoint(relativePos.x, relativePos.y);

                // Initialize tangent based on surrounding points
                if (insertPos > 0 && insertPos < curvePoints.size()) {
                    // Between two points - calculate a smooth tangent
                    float dx = curvePoints[insertPos].main.x - curvePoints[insertPos - 1].main.x;
                    float dy = curvePoints[insertPos].main.y - curvePoints[insertPos - 1].main.y;

                    newPoint.tangent = { dx * 0.3f, dy * 0.3f };
                    // Ensure tangent length is constant
                    float length = sqrt(newPoint.tangent.x * newPoint.tangent.x + newPoint.tangent.y * newPoint.tangent.y);
                    if (length > 0.001f) {
                        newPoint.tangent.x = (newPoint.tangent.x / length) * TANGENT_LENGTH;
                        newPoint.tangent.y = (newPoint.tangent.y / length) * TANGENT_LENGTH;
                    }
                }

                curvePoints.insert(curvePoints.begin() + insertPos, newPoint);
                changed = true;
            }
            lastClickTime = currentTime;
            lastClickPos = mouse;
        }

        // Handle grabbers and interactions
        static int draggedPoint = -1;
        static bool draggingTangent = false;

        if (!IsMouseDown(0)) {
            draggedPoint = -1;
            draggingTangent = false;
        }

        // Check for grabber interactions
        for (int i = 0; i < curvePoints.size(); ++i) {
            // Convert to screen coordinates
            ImVec2 mainPos = ImVec2(curvePoints[i].main.x, 1 - curvePoints[i].main.y) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 tangentPos = ImVec2(
                curvePoints[i].main.x + curvePoints[i].tangent.x,
                1 - (curvePoints[i].main.y + curvePoints[i].tangent.y)
            ) * (bb.Max - bb.Min) + bb.Min;

            float mainDist = Distance({ mouse.x, mouse.y }, { mainPos.x, mainPos.y });
            float tangentDist = Distance({ mouse.x, mouse.y }, { tangentPos.x, tangentPos.y });

            // Handle right-click deletion (only for main points and only if we have more than 2 points)
            if (mainDist < GRAB_RADIUS && IsMouseClicked(1) && curvePoints.size() > 2) {
                curvePoints.erase(curvePoints.begin() + i);
                changed = true;
                break;
            }

            // Handle dragging
            if (IsMouseDown(0)) {
                if (draggedPoint == -1) {
                    if (tangentDist < TANGENT_RADIUS) {
                        draggedPoint = i;
                        draggingTangent = true;
                    }
                    else if (mainDist < GRAB_RADIUS) {
                        draggedPoint = i;
                        draggingTangent = false;
                    }
                }

                if (draggedPoint == i) {
                    ImVec2 delta = GetIO().MouseDelta;
                    ImVec2 scaledDelta = delta / Canvas;
                    scaledDelta.y = -scaledDelta.y; // Flip Y

                    if (draggingTangent) {
                        // Calculate new tangent direction from main point to mouse
                        ImVec2 mouseRelative = (mouse - bb.Min) / (bb.Max - bb.Min);
                        mouseRelative.y = 1.0f - mouseRelative.y; // Flip Y

                        float dx = mouseRelative.x - curvePoints[i].main.x;
                        float dy = mouseRelative.y - curvePoints[i].main.y;

                        // Normalize to constant length
                        float length = sqrt(dx * dx + dy * dy);
                        if (length > 0.001f) {
                            curvePoints[i].tangent.x = (dx / length) * TANGENT_LENGTH;
                            curvePoints[i].tangent.y = (dy / length) * TANGENT_LENGTH;
                        }
                    }
                    else {
                        // Move main point
                        curvePoints[i].main.x = ImClamp(curvePoints[i].main.x + scaledDelta.x, 0.0f, 1.0f);
                        curvePoints[i].main.y = ImClamp(curvePoints[i].main.y + scaledDelta.y, 0.0f, 1.0f);
                    }
                    changed = true;
                }
            }

            // Show tooltips
            if (mainDist < GRAB_RADIUS) {
                SetTooltip("Point (%4.3f, %4.3f)\nRight-click to delete", curvePoints[i].main.x, curvePoints[i].main.y);
            }
            else if (tangentDist < TANGENT_RADIUS) {
                SetTooltip("Tangent Handle\nDrag to adjust curve slope");
            }
        }

        // Draw the curve using the actual curve points
        ImColor curveColor(GetStyle().Colors[ImGuiCol_PlotLines]);
        if (curvePoints.size() >= 2) {
            std::vector<ImVec2> curveScreenPoints;

            // Generate curve points by sampling at regular x intervals
            for (int i = 0; i <= SMOOTHNESS; ++i) {
                float t = (float)i / (float)SMOOTHNESS;
                float y = EvaluateCurve(curvePoints, t);
                y = ImClamp(y, 0.0f, 1.0f);

                ImVec2 screenPoint = ImVec2(t, 1 - y) * (bb.Max - bb.Min) + bb.Min;
                curveScreenPoints.push_back(screenPoint);
            }

            // Draw the curve segments
            for (int i = 0; i < curveScreenPoints.size() - 1; ++i) {
                DrawList->AddLine(curveScreenPoints[i], curveScreenPoints[i + 1], curveColor, CURVE_WIDTH);
            }
        }

        // Draw tangent lines and points
        ImVec4 white(GetStyle().Colors[ImGuiCol_Text]);
        float luma = IsItemActive() || IsItemHovered() ? 0.8f : 0.6f;
        ImVec4 mainColor(1.00f, 0.2f, 0.2f, luma);
        ImVec4 tangentColor(0.2f, 0.8f, 1.0f, luma);
        ImVec4 lineColor(0.6f, 0.6f, 0.6f, luma * 0.7f);

        for (int i = 0; i < curvePoints.size(); ++i) {
            ImVec2 mainPos = ImVec2(curvePoints[i].main.x, 1 - curvePoints[i].main.y) * (bb.Max - bb.Min) + bb.Min;
            ImVec2 tangentPos = ImVec2(
                curvePoints[i].main.x + curvePoints[i].tangent.x,
                1 - (curvePoints[i].main.y + curvePoints[i].tangent.y)
            ) * (bb.Max - bb.Min) + bb.Min;

            // Draw tangent line
            DrawList->AddLine(mainPos, tangentPos, ImColor(lineColor), LINE_WIDTH);

            // Draw main point
            DrawList->AddCircleFilled(mainPos, GRAB_RADIUS, ImColor(white));
            DrawList->AddCircleFilled(mainPos, GRAB_RADIUS - GRAB_BORDER, ImColor(mainColor));

            // Draw tangent handle
            DrawList->AddCircleFilled(tangentPos, TANGENT_RADIUS, ImColor(white));
            DrawList->AddCircleFilled(tangentPos, TANGENT_RADIUS - 1, ImColor(tangentColor));
        }

        // Instructions text
        if (hovered) {
            ImVec2 textPos = ImVec2(bb.Min.x + 5, bb.Min.y + 5);
            DrawList->AddText(textPos, GetColorU32(ImGuiCol_Text, 0.8f), "Double-click: Add point");
            textPos.y += GetTextLineHeight();
            DrawList->AddText(textPos, GetColorU32(ImGuiCol_Text, 0.8f), "Right-click point: Delete");
            textPos.y += GetTextLineHeight();
            DrawList->AddText(textPos, GetColorU32(ImGuiCol_Text, 0.8f), "Drag blue handles: Adjust curve");
        }

        return changed;
    }
}