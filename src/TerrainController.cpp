#include "Controllers/TerrainController.h"

TerrainController::TerrainController(Renderer* renderer)
    : GeneratorController(renderer) {
    // Initialize the terrain generator and UI
    generator = TerrainGenerator();
    terrainUI = new TerrainUI(this);
}

TerrainController::~TerrainController() {
}

void TerrainController::Update() {
    // Update the terrain based on the current UI parameters
    TerrainUtilities::TerrainData params = terrainUI->GetParameters();
    generator.SetParameters(params);
    Generate();
}

void TerrainController::Generate() {
    // Generate the terrain mesh and push it to the renderer
    Mesh* terrainMesh = generator.Generate();
    if (terrainMesh) {
        // renderer->AddToRenderQueue(terrainMesh); // Add the mesh to the renderer's queue
    }
}

void TerrainController::DisplayUI() {
    // Display the terrain UI
    terrainUI->DisplayUI();
}