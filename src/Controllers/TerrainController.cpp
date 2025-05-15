#include "TerrainController.h"

TerrainController::TerrainController(Renderer* renderer)
    : GeneratorController(renderer) {
    // Initialize the terrain generator and UI
    generator = TerrainGenerator();
    terrainUI = new TerrainUI(this);
}

TerrainController::~TerrainController() {

}

void TerrainController::Update() {
    renderer->AddToRenderQueue(generator.GetMesh());
}

void TerrainController::UpdateParameters() {
    TerrainUtilities::TerrainData params = terrainUI->GetParameters();
    generator.SetParameters(params);
}

void TerrainController::Generate() {
    UpdateParameters();

    generator.Generate();
}

void TerrainController::DisplayUI() {
    terrainUI->DisplayUI();
}