#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
#include "../Utils/OBJExporter.h"
#include "../UI/SpectrumUI.h"
#include <GLFW/glfw3.h>
#include "../Core/Camera.h"
#include <iostream>
#include <fstream>

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
    void exportTerrain(string format);

    // New: allow external code to inject TerrainGenerator
    void setTerrainGenerator(TerrainGenerator* tg);

    // Theme functions
    void switchTheme();

private:
    Camera* camera;
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();
    void renderSceneOverlay(float* viewMatrix, float cameraDistance);
    void renderSceneOverlay();

    TerrainGenerator* terrainGen = nullptr; // Pointer to avoid circular dependency

    // Theme state
    bool isDarkTheme = true;
};
