#pragma once

#include "GeneratorUI.h"
#include "../Core/Engine/Application.h"
#include "../Controllers/TerrainController.h"
#include "../Core/TerrainData.h"
#include "../Utils/FileDialogs.h"

class TerrainController;

class TerrainUI : public GeneratorUI {
public:
    TerrainUI(TerrainController* controller);
    ~TerrainUI() = default;
    void DisplayUI() override;
    void RandomizeSeed() override;

    TerrainUtilities::TerrainData GetParameters() const;

private:
    TerrainUtilities::TerrainData parameters;
    TerrainController* controller;
    bool liveUpdate = true;

    // New tab-based methods
    void DisplayMainSettingsWindow();
    void DisplayTerrainSettingsTab();
    void DisplayAppearanceTab();
    void DisplayDecorationTab();

    void DisplayColorSettings();
    void DisplayTextureLayerSettings();
    void DisplayDecorationSettings();

    void RenderFalloffControls();
    void DisplaySceneViewOverlay();

};
