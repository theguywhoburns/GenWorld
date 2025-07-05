#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
#include "../UI/SpectrumUI.h"
#include <GLFW/glfw3.h>
#include "../Core/Camera.h"
#include <iostream>

class TerrainGenerator;

class UiContext : public IRenderContext {
public:
    ~UiContext() override = default;

    bool init(AppWindow* window) override;
    void shutdown() override;

    void preRender() override;
    void render() override;
    void postRender() override;
    void setCamera(Camera* cam) { camera = cam; }
    void exportMesh(std::string format);

    // Theme functions
    void switchTheme();
    int getMode() const { return mode; }

private:
    Camera* camera;
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();
    void renderSceneOverlay(float* viewMatrix, float cameraDistance);
    void renderSceneOverlay();

    int mode = 0; // 0 = Block Generation, 1 = Terrain Generation

    // Theme state
    bool isDarkTheme = true;
};
