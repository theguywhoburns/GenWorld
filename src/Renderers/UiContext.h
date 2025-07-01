#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
#include "../UI/SpectrumUI.h"
#include <GLFW/glfw3.h>
#include "../Core/Camera.h"
#include <iostream>
#include <fstream>

class UiContext : public IRenderContext {
public:
    ~UiContext() override = default;

    bool init(AppWindow* window) override;
    void shutdown() override;

    void preRender() override;
    void render() override;
    void postRender() override;
    void setCamera(Camera* cam) { camera = cam; }

    // Theme functions
    void switchTheme();

private:
    Camera* camera;
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();
    void renderSceneOverlay(float* viewMatrix, float cameraDistance);
    void renderSceneOverlay();

    // Theme state
    bool isDarkTheme = true;
};
