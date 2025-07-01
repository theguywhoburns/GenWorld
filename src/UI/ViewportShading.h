#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <functional>
#include <glm/glm.hpp>

enum class ViewportShadingMode {
    Wireframe,
    SolidColor,
    RenderedNoLights,
    RenderedWithLights
};

struct ShadingParameters {
    ViewportShadingMode mode;

    // Wireframe parameters
    float wireframeWidth = 1.0f;
    glm::vec3 wireframeColor = glm::vec3(0.0f);
    bool useFilledWireframe = true;
    glm::vec3 filledWireframeColor = glm::vec3(0.4f);

    // Solid color parameters
    glm::vec3 solidColor = glm::vec3(0.4f);

    // Lighting parameters (only for RenderedWithLights)
    glm::vec3 lightDirection = glm::vec3(-1.0, -1.0, -0.5);
    glm::vec3 lightColor = glm::vec3(1.0f);
    float ambient = 0.5f;
    float diffuse = 1.0f;
};

class ShadingPanel {
public:
    ShadingPanel();

    void render();
    void setOnParametersChanged(std::function<void(const ShadingParameters&)> callback);

    const ShadingParameters& getParameters() const;
    void setParameters(const ShadingParameters& newParams);

private:
    ShadingParameters params;
    std::function<void(const ShadingParameters&)> onParametersChanged;

    bool parametersCollapsed = true;

    void triggerCallback();
    void resetAllDefaults();

    // New Blender-style rendering methods
    void renderModeOverlay();
    void renderModeButtons();
    void renderSidePanel();
    void renderCollapsedPanel();
    void renderExpandedPanel();

    // Mode-specific UI methods
    void renderModeSpecificUI();
    void renderWireframeUI();
    void renderSolidColorUI();
    void renderNoLightsUI();
    void renderWithLightsUI();
};