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

    void RenderFalloffControls();
    void DisplaySceneViewOverlay();
    void DisplayTerrainSettingsUI();
    void DisplayColorSettingsUI();
    void DisplayTextureLayerSettings();
    void DisplayDecorationSettings();

};
