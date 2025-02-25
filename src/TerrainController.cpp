#include "Controllers/TerrainController.h"

TerrainController::TerrainController(Renderer* renderer)
    : GeneratorController(renderer) {
    // Initialize the terrain generator and UI
    generator = TerrainGenerator();
    terrainUI = new TerrainUI(this);
    terrainMesh = nullptr;
}

TerrainController::~TerrainController() {
    // Clean up the terrain mesh and UI
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
    // Update the terrain based on the current UI parameters
    TerrainUtilities::TerrainData params = terrainUI->GetParameters();
    generator.SetParameters(params);
}

void TerrainController::Generate() {
    UpdateParameters();

    // Clean up the old mesh
    if (terrainMesh != nullptr) {
        delete terrainMesh;
    }

    terrainMesh = generator.Generate();
}

void TerrainController::DisplayUI() {
    // Display the terrain UI
    terrainUI->DisplayUI();
}