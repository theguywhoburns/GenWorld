#pragma once

#include "GeneratorUI.h"
#include "../Controllers/TerrainController.h"
#include "../Core/TerrainData.h"

// Forward declaration
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
    
    void RenderFalloffControls();

};
