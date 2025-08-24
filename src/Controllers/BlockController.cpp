#include <GenWorld/Controllers/BlockController.h>
#include <GenWorld/Drawables/BlockMesh.h>
#include <GenWorld/Drawables/Mesh.h>
#include <GenWorld/Drawables/Model.h>
#include <GenWorld/Generators/BlockGenerator.h>
#include <GenWorld/Renderers/Renderer.h>
#include <GenWorld/UI/BlockUI.h>
#include <iostream>

BlockController::BlockController(Renderer *renderer)
    : GeneratorController(renderer) {

  generator = new BlockGenerator(this);
  blockUI = new BlockUI(this);
  blockMesh = nullptr;
}

BlockController::~BlockController() {
  if (blockUI != nullptr) {
    delete blockUI;
  }

  if (generator != nullptr) {
    delete generator;
  }
}

void BlockController::DisplayUI() { blockUI->DisplayUI(); }

void BlockController::Update() {
  if (blockMesh != nullptr) {
    renderer->AddToRenderQueue(blockMesh);
  }
}

void BlockController::RandomizeSeed() { blockUI->RandomizeSeed(); }

IGeneratorStrategy &BlockController::getGenerator() { return *generator; }

void BlockController::UpdateParameters() {
  BlockUtilities::BlockData params = blockUI->GetParameters();
  generator->SetParameters(params);
}

void BlockController::LoadModel(const std::string &filepath) {
  try {
    auto model = std::make_shared<Model>(filepath.c_str());
    if (blockUI) {
      blockUI->OnModelLoaded(model, filepath);
    }
  } catch (const std::exception &e) {
    if (blockUI) {
      blockUI->OnModelLoadError(e.what());
    }
  }
}

void BlockController::Generate() {
  UpdateParameters();

  Transform currentTransform;
  bool hasExistingTransform = false;

  if (blockMesh != nullptr) {
    currentTransform = blockMesh->getTransform();
    hasExistingTransform = true;
    delete blockMesh;
    blockMesh = nullptr;
  }

  generator->Generate();
  blockMesh = generator->GetMesh();

  // Restore transform to new mesh
  if (hasExistingTransform && blockMesh) {
    blockMesh->setTransform(currentTransform);
  }
}