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
    ~TerrainUI();
    void DisplayUI() override;
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

    // Renamed methods (remove the old ones and add these)
    void DisplayColorSettings();           // was DisplayColorSettingsUI()
    void DisplayTextureLayerSettings();    // was DisplayTextureLayerSettings() (same name)
    void DisplayDecorationSettings();      // was DisplayDecorationSettings() (same name)

    // Keep these existing methods unchanged:
    void RenderFalloffControls();
    void DisplaySceneViewOverlay();

};
