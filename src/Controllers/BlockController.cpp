#include "BlockController.h"
#include "../UI/BlockUI.h"

BlockController::BlockController(Renderer* renderer)
    : GeneratorController(renderer) {
    // Initialize the block generator and UI
    generator = BlockGenerator();
    blockUI = new BlockUI(this);
    blockMesh = nullptr;
}

BlockController::~BlockController() {
    if (blockMesh != nullptr) {
        delete blockMesh;
    }
}


void BlockController::Update() {
    if (blockMesh != nullptr) {
        renderer->AddToRenderQueue(blockMesh);
    }
}

void BlockController::UpdateParameters() {
    BlockUtilities::BlockData params = blockUI->GetParameters();
    generator.SetParameters(params);
}

void BlockController::Generate() {
    UpdateParameters();

    // Clean up the old mesh
    if (blockMesh != nullptr) {
        delete blockMesh;
    }

    blockMesh = generator.Generate();
}