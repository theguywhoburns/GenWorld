#include "TerrainController.h"

TerrainController::TerrainController(Renderer* renderer)
    : GeneratorController(renderer) {
    // Initialize the terrain generator and UI
    generator = TerrainGenerator();
    terrainUI = new TerrainUI(this);
    terrainMesh = nullptr;
}

TerrainController::~TerrainController() {
    if (terrainMesh != nullptr) {
        delete terrainMesh;
    }
}

void TerrainController::Update() {
    if (terrainMesh != nullptr) {
        renderer->AddToRenderQueue(terrainMesh);
    }
}

void TerrainController::UpdateParameters() {
    TerrainUtilities::TerrainData params = terrainUI->GetParameters();
    generator.SetParameters(params);
}

void TerrainController::Generate() {
    UpdateParameters();

    terrainMesh = generator.Generate();
}

void TerrainController::DisplayUI() {
    terrainUI->DisplayUI();
}