#include <GenWorld/Controllers/TerrainController.h>

TerrainController::TerrainController(Renderer *renderer)
    : GeneratorController(renderer) {
  // Initialize the terrain generator and UI
  generator = TerrainGenerator();
  terrainUI = new TerrainUI(this);
}

void TerrainController::Update() {
  Mesh *mesh = generator.GetMesh();
  if (mesh == nullptr) {
    return; // No mesh to render
  }

  renderer->AddToRenderQueue(mesh);
}

void TerrainController::RandomizeSeed() { terrainUI->RandomizeSeed(); }

IGeneratorStrategy &TerrainController::getGenerator() { return generator; }

void TerrainController::UpdateParameters() {
  TerrainUtilities::TerrainData params = terrainUI->GetParameters();
  generator.SetParameters(params);
}

void TerrainController::Generate() {
  if (generator.GetParameters() == terrainUI->GetParameters())
    return;

  UpdateParameters();

  generator.Generate();
}

void TerrainController::DisplayUI() { terrainUI->DisplayUI(); }