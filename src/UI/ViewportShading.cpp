#include "ViewportShading.h"

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
        contentRegionMin.y + 25
    );

    ImGui::SetCursorPos(overlayPos);

    // Style overrides for the mode selector
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.12f, 0.15f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.35f, 0.8f));

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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.55f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.45f, 0.75f, 1.0f));
        }
        else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.35f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.2f, 1.0f));
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
        ImVec2 overlaySize(18, 40);
        ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
        ImVec2 overlayPos(
            contentRegionMin.x + 10,
            contentRegionMin.y + 70
        );
        ImGui::SetCursorPos(overlayPos);

        // Style overrides
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);

        ImGui::BeginChild("SceneViewOverlay", overlaySize,
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoTitleBar);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.3f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.4f, 1.0f));

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
        ImVec2 overlaySize(400, 300);
        ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
        ImVec2 overlayPos(
            contentRegionMin.x + 10,
            contentRegionMin.y + 70
        );
        ImGui::SetCursorPos(overlayPos);

        // Style overrides
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.18f, 0.95f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

        ImGui::BeginChild("SceneViewOverlay", overlaySize,
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoTitleBar);


        // Header with collapse button
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.3f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.35f, 0.4f, 1.0f));

        if (ImGui::Button("<", ImVec2(20, 20))) {
            parametersCollapsed = true;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Collapse Panel");
        }

        ImGui::SameLine();
        ImGui::Text("Shading Parameters");

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();

        ImGui::Separator();
        ImGui::Spacing();

        // Current mode display
        const char* modeNames[] = {
            "Wireframe", "Solid Color", "Material Preview", "Rendered"
        };

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
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
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Wireframe Settings");
    ImGui::Spacing();

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
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Solid Color Settings");
    ImGui::Spacing();

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
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Material Preview");
    ImGui::Separator();

    // Show informational text
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.6f, 1.0f));
    ImGui::TextWrapped("This mode simulates material properties without lighting. It is useful for previewing textures and material settings without the influence of scene lighting.");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Text("No additional parameters required.");
}

void ShadingPanel::renderWithLightsUI() {
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.3f, 1.0f), "Lighting Settings");
    ImGui::Spacing();

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
    ImGui::SameLine();
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