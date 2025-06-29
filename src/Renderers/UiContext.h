#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
#include <GLFW/glfw3.h>
#include "../Core/Camera.h"
#include <iostream>
#include <fstream>

class UiContext : public IRenderContext {
public:
    ~UiContext() override = default;
    
    bool init(Window* window) override;
    void shutdown() override;

    void preRender() override;
    void render() override;
    void postRender() override;
    void setCamera(Camera* cam) { camera = cam; }

private:
    Camera* camera;
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();
    void renderSceneOverlay(float* viewMatrix, float cameraDistance);
    void renderSceneOverlay();

};