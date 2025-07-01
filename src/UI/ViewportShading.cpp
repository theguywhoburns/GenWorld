#include "ViewportShading.h"
#include "SpectrumUI.h"

ShadingPanel::ShadingPanel() {
    // Initialize with default values
    params.mode = ViewportShadingMode::RenderedNoLights;
    resetAllDefaults();
    triggerCallback();
}

void ShadingPanel::setOnParametersChanged(std::function<void(const ShadingParameters&)> callback) {
    onParametersChanged = callback;
}

void ShadingPanel::render() {
    renderModeOverlay();
    renderSidePanel();
}

const ShadingParameters& ShadingPanel::getParameters() const {
    return params;
}

void ShadingPanel::setParameters(const ShadingParameters& newParams) {
    params = newParams;
    triggerCallback();
}

void ShadingPanel::triggerCallback() {
    if (onParametersChanged) {
        onParametersChanged(params);
    }
}

void ShadingPanel::renderModeOverlay() {
    const ImU32 flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoDocking;

    ImGuiWindow* sceneWindow = ImGui::FindWindowByName("Scene View");
    if (!sceneWindow) return;

    ImGui::SetNextWindowSize(sceneWindow->Size);
    ImGui::SetNextWindowPos(sceneWindow->Pos);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowViewport(sceneWindow->ViewportId);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("ShadingModeOverlay", nullptr, flags);

    ImVec2 originalPos = ImGui::GetCursorPos();
    ImVec2 overlaySize(140, 36);
    ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

    // Position at top-right of scene view
    ImVec2 overlayPos(
        contentRegionMin.x + 10,
        contentRegionMin.y + 40
    );

    ImGui::SetCursorPos(overlayPos);

    // Style overrides for the mode selector using Spectrum colors
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY75()));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY300()));

    ImGui::BeginChild("ModeSelector", overlaySize,
        ImGuiChildFlags_Borders,
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar);

    renderModeButtons();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::EndChild();

    ImGui::SetCursorPos(originalPos);
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void ShadingPanel::renderModeButtons() {
    // Mode button data
    struct ModeButton {
        ViewportShadingMode mode;
        const char* icon;
        const char* tooltip;
    };

    ModeButton modes[] = {
        { ViewportShadingMode::Wireframe, "W", "Wireframe" },
        { ViewportShadingMode::SolidColor, "S", "Solid" },
        { ViewportShadingMode::RenderedNoLights, "M", "Material Preview" },
        { ViewportShadingMode::RenderedWithLights, "R", "Rendered" }
    };

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    float buttonSize = 32.0f;
    float totalWidth = (buttonSize * 4) + (2 * 3); // 4 buttons + 3 spaces
    float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;
    float startY = (ImGui::GetContentRegionAvail().y - buttonSize) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + startY);

    for (int i = 0; i < 4; i++) {
        if (i > 0) ImGui::SameLine();

        bool isActive = (params.mode == modes[i].mode);

        if (isActive) {
            // Use Spectrum blue colors for active state
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::BLUE500()));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::BLUE600()));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::BLUE400()));
        }
        else {
            // Use Spectrum gray colors for inactive state
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY300()));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY400()));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY200()));
        }

        if (ImGui::Button(modes[i].icon, ImVec2(buttonSize, buttonSize))) {
            params.mode = modes[i].mode;
            triggerCallback();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", modes[i].tooltip);
        }

        ImGui::PopStyleColor(3);
    }

    ImGui::PopStyleVar(2);
}

void ShadingPanel::renderSidePanel() {
    if (parametersCollapsed) {
        renderCollapsedPanel();
    }
    else {
        renderExpandedPanel();
    }
}

void ShadingPanel::renderCollapsedPanel() {
    ImGuiWindow* sceneWindow = ImGui::FindWindowByName("Scene View");
    if (!sceneWindow) return;

    ImGui::SetNextWindowPos(sceneWindow->Pos);
    ImGui::SetNextWindowSize(sceneWindow->Size);
    ImGui::SetNextWindowViewport(sceneWindow->ViewportId);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("ShadingPanelCollapsed", nullptr, flags)) {
        ImVec2 originalPos = ImGui::GetCursorPos();
        ImVec2 overlaySize(30, 38);
        ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
        ImVec2 overlayPos(
            contentRegionMin.x + 10,
            contentRegionMin.y + 85
        );
        ImGui::SetCursorPos(overlayPos);

        // Style overrides using Spectrum colors
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

        ImGui::BeginChild("SceneViewOverlay", overlaySize,
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoTitleBar);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        // Use Spectrum gray colors for the expand button
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY400()));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY500()));

        if (ImGui::Button(">", overlaySize)) {
            parametersCollapsed = false;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Expand Shading Parameters");
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        
        ImGui::EndChild();

        
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void ShadingPanel::renderExpandedPanel() {
    ImGuiWindow* sceneWindow = ImGui::FindWindowByName("Scene View");
    if (!sceneWindow) return;

    ImGui::SetNextWindowPos(sceneWindow->Pos);
    ImGui::SetNextWindowSize(sceneWindow->Size);
    ImGui::SetNextWindowViewport(sceneWindow->ViewportId);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    if (ImGui::Begin("ShadingPanelExpanded", nullptr, flags)) {
        ImVec2 originalPos = ImGui::GetCursorPos();
        
        // Calculate available space and set reasonable constraints
        ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();
        float availableHeight = contentRegionMax.y - contentRegionMin.y;
        
        // Content-aware sizing with threshold for scrolling
        float overlayWidth = 400.0f;
        float maxHeight = availableHeight - 130.0f;
        float contentThreshold = 405.0f;  // Threshold where scrolling kicks in
        
        // If we have less available space than threshold, use available space
        if (maxHeight < contentThreshold) {
            maxHeight = ImMax(maxHeight, 280.0f);  // Minimum usable height
        } else {
            maxHeight = contentThreshold;  // Use threshold as max before scrolling
        }
        
        ImVec2 overlaySize(overlayWidth, maxHeight);
        ImVec2 overlayPos(
            contentRegionMin.x + 10,
            contentRegionMin.y + 85
        );
        ImGui::SetCursorPos(overlayPos);

        // Style overrides using Spectrum colors
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY75()));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

        // Add scrollbar only if content would exceed our threshold
        ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoTitleBar;
        if (maxHeight <= contentThreshold && availableHeight - 130.0f > contentThreshold) {
            childFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
        }

        ImGui::BeginChild("SceneViewOverlay", overlaySize,
            ImGuiChildFlags_Borders, childFlags);


        // Header with collapse button
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        // Use Spectrum gray colors for the collapse button
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY400()));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY500()));

        if (ImGui::Button("<", ImVec2(30, 38))) {
            parametersCollapsed = true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Collapse Panel");
        }

        ImGui::SameLine();
        // Use bold font for the main title
        ImGui::Spectrum::BeginTitleFont();
        ImGui::Text("Shading Parameters");
        ImGui::Spectrum::EndTitleFont();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        ImGui::Separator();
        ImGui::Spacing();

        // Current mode display
        const char* modeNames[] = {
            "Wireframe", "Solid Color", "Material Preview", "Rendered"
        };

        // Use Spectrum text color for current mode
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::BLUE600()));
        ImGui::Text("Current Mode: %s", modeNames[static_cast<int>(params.mode)]);
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Spacing();

        // Mode-specific parameters
        renderModeSpecificUI();

        // Reset all button at the bottom
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Reset All Defaults", ImVec2(-1, 0))) {
            resetAllDefaults();
            triggerCallback();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void ShadingPanel::renderModeSpecificUI() {
    switch (params.mode) {
    case ViewportShadingMode::Wireframe:
        renderWireframeUI();
        break;

    case ViewportShadingMode::SolidColor:
        renderSolidColorUI();
        break;

    case ViewportShadingMode::RenderedNoLights:
        renderNoLightsUI();
        break;

    case ViewportShadingMode::RenderedWithLights:
        renderWithLightsUI();
        break;
    }
}

void ShadingPanel::renderWireframeUI() {
    // Use Spectrum section title with bold font
    ImGui::Spectrum::SectionTitle("Wireframe Settings");

    if (ImGui::SliderFloat("Line Width", &params.wireframeWidth, 0.1f, 5.0f, "%.1f")) {
        triggerCallback();
    }

    if (ImGui::ColorEdit3("Wire Color", &params.wireframeColor[0])) {
        triggerCallback();
    }

    // Compact wire color presets
    ImGui::Text("Presets:");
    if (ImGui::SmallButton("White")) {
        params.wireframeColor = glm::vec3(1.0f);
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Black")) {
        params.wireframeColor = glm::vec3(0.0f);
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Green")) {
        params.wireframeColor = glm::vec3(0.0f, 1.0f, 0.0f);
        triggerCallback();
    }

    ImGui::Spacing();

    // Filled wireframe option
    if (ImGui::Checkbox("Use Filled Wireframe", &params.useFilledWireframe)) {
        triggerCallback();
    }

    if (params.useFilledWireframe) {
        ImGui::Indent();
        if (ImGui::ColorEdit3("Fill Color", &params.filledWireframeColor[0])) {
            triggerCallback();
        }
        if (ImGui::SmallButton("Gray")) {
            params.filledWireframeColor = glm::vec3(0.4f);
            triggerCallback();
        }
        ImGui::Unindent();
    }
}

void ShadingPanel::renderSolidColorUI() {
    // Use Spectrum section title with bold font
    ImGui::Spectrum::SectionTitle("Solid Color Settings");

    if (ImGui::ColorEdit3("Color", &params.solidColor[0])) {
        triggerCallback();
    }

    // Compact preset colors
    ImGui::Text("Presets:");
    if (ImGui::Button("Clay")) {
        params.solidColor = glm::vec3(0.8f, 0.6f, 0.4f);
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::Button("Plastic")) {
        params.solidColor = glm::vec3(0.7f);
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::Button("Metal")) {
        params.solidColor = glm::vec3(0.5f, 0.5f, 0.6f);
        triggerCallback();
    }
}

void ShadingPanel::renderNoLightsUI() {
    // Use Spectrum section title with bold font
    ImGui::Spectrum::SectionTitle("Material Preview");

    // Show informational text using Spectrum color
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY600()));
    ImGui::TextWrapped("This mode simulates material properties without lighting. It is useful for previewing textures and material settings without the influence of scene lighting.");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Text("No additional parameters required.");
}

void ShadingPanel::renderWithLightsUI() {
    // Use Spectrum section title with bold font
    ImGui::Spectrum::SectionTitle("Lighting Settings");

    // Light Direction
    if (ImGui::SliderFloat3("Light Direction", &params.lightDirection[0], -1.0f, 1.0f)) {
        triggerCallback();
    }

    // Compact light direction presets
    if (ImGui::SmallButton("Top")) {
        params.lightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Front")) {
        params.lightDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Side")) {
        params.lightDirection = glm::vec3(-1.0f, -0.5f, 0.0f);
        triggerCallback();
    }

    ImGui::Spacing();

    // Light Color
    if (ImGui::ColorEdit3("Light Color", &params.lightColor[0])) {
        triggerCallback();
    }

    // Ambient and Diffuse intensity
    if (ImGui::SliderFloat("Ambient", &params.ambient, 0.0f, 1.0f, "%.2f")) {
        triggerCallback();
    }

    if (ImGui::SliderFloat("Diffuse", &params.diffuse, 0.0f, 3.0f, "%.2f")) {
        triggerCallback();
    }

    // Compact lighting presets
    ImGui::Spacing();
    ImGui::Text("Presets:");

    // First row of presets
    if (ImGui::Button("Standard")) {
        params.lightColor = glm::vec3(1.0f);
        params.ambient = 0.5f;
        params.diffuse = 1.0f;
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::Button("Soft")) {
        params.lightColor = glm::vec3(1.0f);
        params.ambient = 0.3f;
        params.diffuse = 0.7f;
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::Button("Dramatic")) {
        params.lightColor = glm::vec3(1.0f);
        params.ambient = 0.05f;
        params.diffuse = 2.0f;
        triggerCallback();
    }

    // Second row of presets
    if (ImGui::Button("Warm")) {
        params.lightColor = glm::vec3(1.0f, 0.9f, 0.7f);
        params.ambient = 0.2f;
        params.diffuse = 1.2f;
        triggerCallback();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cool")) {
        params.lightColor = glm::vec3(0.7f, 0.8f, 1.0f);
        params.ambient = 0.15f;
        params.diffuse = 0.8f;
        triggerCallback();
    }
}

void ShadingPanel::resetAllDefaults() {
    // Wireframe defaults
    params.wireframeWidth = 1.0f;
    params.wireframeColor = glm::vec3(0.0f);
    params.useFilledWireframe = true;
    params.filledWireframeColor = glm::vec3(0.4f);

    // Solid color defaults
    params.solidColor = glm::vec3(0.4f);

    // Lighting defaults
    params.lightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    params.lightColor = glm::vec3(1.0f);
    params.ambient = 0.5f;
    params.diffuse = 1.0f;
}