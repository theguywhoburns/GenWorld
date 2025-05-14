#include "BlockUI.h"


BlockUI::BlockUI(BlockController* controller) : controller(controller) {
    // Block Data
    parameters.width = 100;
    parameters.length = 100;
    parameters.cellSize = 1;
    parameters.numCellsWidth = static_cast<unsigned int>(parameters.width / parameters.cellSize);
    parameters.numCellsLength = static_cast<unsigned int>(parameters.length / parameters.cellSize);
    parameters.halfWidth = parameters.width / 2.0f;
    parameters.halfLength = parameters.length / 2.0f;
    parameters.stepX = parameters.width / (parameters.numCellsWidth - 1);
    parameters.stepZ = parameters.length / (parameters.numCellsLength - 1);
}


void BlockUI::DisplayUI() {
    ImGui::Begin("Block Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::Text("Block Settings");
    ImGui::DragFloat("Width", &parameters.width, 0.1f, 1, 100);
    ImGui::DragFloat("Length", &parameters.length, 0.1f, 1, 100);
    ImGui::SliderInt("Division Size", &parameters.cellSize, 1, 10);

    if (ImGui::Button("Generate", ImVec2(200, 40))) {
        controller->Generate();
    }

    ImGui::End();
}

BlockUI::~BlockUI() {
    // Destructor implementation
}