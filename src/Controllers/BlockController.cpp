#include "BlockController.h"
#include "../UI/BlockUI.h"
#include "../Generators/BlockGenerator.h"
#include "../Drawables/Model.h"
#include "../Renderers/Renderer.h"
#include "../Drawables/Mesh.h"
#include <iostream>

BlockController::BlockController(Renderer* renderer)
    : GeneratorController(renderer) {

    generator = new BlockGenerator(this);
    blockUI = new BlockUI(this);
    blockMesh = nullptr;
}

BlockController::~BlockController() {
    if (blockMesh != nullptr) {
        delete blockMesh;
    }
    
    if (blockUI != nullptr) {
        delete blockUI;
    }
    
    if (generator != nullptr) {
        delete generator;
    }
}

void BlockController::DisplayUI() {
    blockUI->DisplayUI();
}

void BlockController::Update() {
    if (blockMesh != nullptr) {
        renderer->AddToRenderQueue(blockMesh);
    }
}

void BlockController::UpdateParameters() {
    BlockUtilities::BlockData params = blockUI->GetParameters();
    generator->SetParameters(params);
}

void BlockController::LoadModel(const std::string& filepath) {
    try {
        auto model = std::make_shared<Model>(filepath.c_str());
        if (blockUI) {
            blockUI->OnModelLoaded(model, filepath);
        }
    }
    catch (const std::exception& e) {
        if (blockUI) {
            blockUI->OnModelLoadError(e.what());
        }
    }
}

void BlockController::Generate() {
    UpdateParameters();
    
    // Trigger auto-detection of cell size if not already done
    if (generator) {
        generator->DetectCellSizeFromAssets();
    }

    ShaderManager* shaderManager = ShaderManager::GetInstance();
    shaderManager->getShader("unshaded")->use();
    
    if (blockMesh != nullptr) {
        delete blockMesh;
        blockMesh = nullptr;
        renderer->ClearQueue();
    }
    
    generator->Generate();
    blockMesh = generator->GetMesh();
}