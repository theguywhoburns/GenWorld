#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
#include "../Utils/OBJExporter.h"
#include "../Utils/FBXExporter.h"
#include <GLFW/glfw3.h>

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
    void exportTerrain(string format);

    // New: allow external code to inject TerrainGenerator
    void setTerrainGenerator(TerrainGenerator* tg);

private:
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();
    void renderUniversalButtons();

    TerrainGenerator* terrainGen = nullptr; // Pointer to avoid circular dependency

};