#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "BlockUI.h"
#include "IBlockUIController.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>

// ONLY include what we absolutely need for file dialogs
#include "../Core/Engine/Application.h"
#include "../Utils/FileDialogs.h"
#include "../Drawables/Model.h"

BlockUI::BlockUI(IBlockUIController* controller) : controller(controller) {
    // Initialize with block count instead of world units
    parameters.gridWidth = 20;     // 20 blocks wide
    parameters.gridLength = 20;    // 20 blocks long
    parameters.blockScale = 1.0f;
}

void BlockUI::DisplayUI() {
    ImGui::Begin("Block Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    
    DisplayGridSettings();
    ImGui::Separator();
    DisplayAssetManagement();
    ImGui::Separator();
    DisplayBlockSettings();
    ImGui::Separator();
    DisplayDirectionalConstraints();
    
    if (ImGui::Button("Generate", ImVec2(200, 40))) {
        if (controller) {
            controller->Generate();
        }
    }

    ImGui::End();
}

void BlockUI::DisplayGridSettings() {
    ImGui::Text("Grid Settings");
    
    int oldGridWidth = parameters.gridWidth;
    int oldGridLength = parameters.gridLength;
    
    ImGui::DragInt("Grid Width (blocks)", reinterpret_cast<int*>(&parameters.gridWidth), 1, 5, 100);
    ImGui::DragInt("Grid Length (blocks)", reinterpret_cast<int*>(&parameters.gridLength), 1, 5, 100);
    
    ImGui::Text("Grid Info:");
    ImGui::Text("Grid size: %u x %u = %u total blocks", 
                parameters.gridWidth, parameters.gridLength, 
                parameters.gridWidth * parameters.gridLength);
    
    // Show detected cell dimensions if available
    if (parameters.dimensionsDetected) {
        ImGui::Separator();
        ImGui::Text("Auto-detected from first model:");
        ImGui::Text("Cell width: %.2f units", parameters.cellWidth);
        ImGui::Text("Cell length: %.2f units", parameters.cellLength);
        ImGui::Text("World size: %.1f x %.1f units", 
                    parameters.worldWidth, parameters.worldLength);
    }
}

void BlockUI::DisplayBlockSettings() {
    ImGui::Text("Block Settings");
    
    // Block scale control
    ImGui::DragFloat("Block Scale", &parameters.blockScale, 0.01f, 0.1f, 5.0f);
    
    // Reset to defaults button
    if (ImGui::Button("Reset to Default")) {
        parameters.blockScale = 1.0f;
    }
    
    // Visual preview
    ImGui::Separator();
    ImGui::Text("Preview");
    ImGui::Text("Block scale: %.2f", parameters.blockScale);
}

void BlockUI::OpenModelFileDialog() {
    GLFWwindow* windowHandle = Application::GetInstance()->GetWindow()->getNativeWindow();
    
    std::string filepath = Utils::FileDialogs::OpenFile(
        "3D Model Files", 
        "Model Files (*.obj;*.fbx;*.gltf;*.glb)\0*.obj;*.fbx;*.gltf;*.glb\0All Files\0*.*\0",
        windowHandle
    );
    
    if (!filepath.empty() && controller) {
        // Let the controller handle the actual loading
        controller->LoadModel(filepath);
    }
}

// Callback methods for controller to report results
void BlockUI::OnModelLoaded(std::shared_ptr<Model> model, const std::string& filepath) {
    AssetInfo newAsset;
    newAsset.id = loadedAssets.empty() ? 0 : loadedAssets.back().id + 1;
    newAsset.name = "Model_" + std::to_string(newAsset.id);
    newAsset.blockPath = filepath;
    newAsset.model = model;
    loadedAssets.push_back(newAsset);
}

void BlockUI::OnModelLoadError(const std::string& error) {
    lastError = error;
    showModelError = true;
}

void BlockUI::DisplayAssetManagement() {
    ImGui::Text("Asset Management");
    
    if (ImGui::Button("Load Model", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        OpenModelFileDialog();
    }
    
    // Error popup
    if (showModelError) {
        ImGui::OpenPopup("Model Load Error");
        showModelError = false;
    }
    
    if (ImGui::BeginPopupModal("Model Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Failed to load model file!");
        ImGui::Text("%s", lastError.c_str());
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    ImGui::Text("Loaded Assets");
    
    if (loadedAssets.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No assets loaded.");
    } else {
        if (ImGui::BeginTable("AssetsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Model Path");
            ImGui::TableHeadersRow();
            
            for (size_t i = 0; i < loadedAssets.size(); i++) {
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%d", loadedAssets[i].id);
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", loadedAssets[i].name.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%s", GetFileName(loadedAssets[i].blockPath).c_str());
            }
            ImGui::EndTable();
        }
    }
}

std::string BlockUI::GetFileName(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return filepath.substr(lastSlash + 1);
    }
    return filepath;
}

void BlockUI::DisplayDirectionalConstraints() {
    ImGui::Text("Directional Block Constraints");
    ImGui::Separator();
    
    if (loadedAssets.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Load assets first to set up constraints.");
        return;
    }
    
    static int selectedAssetIndex = 0;
    static int selectedSideIndex = 0;
    
    // Ensure selectedAssetIndex is within bounds
    if (selectedAssetIndex >= static_cast<int>(loadedAssets.size())) {
        selectedAssetIndex = 0;
    }
    
    // Asset selection
    std::vector<const char*> assetNames;
    for (const auto& asset : loadedAssets) {
        assetNames.push_back(asset.name.c_str());
    }
    
    if (!assetNames.empty()) {
        ImGui::Combo("Block Type", &selectedAssetIndex, assetNames.data(), static_cast<int>(assetNames.size()));
    }
    
    // Side selection
    const char* sideNames[] = {"Front (+Z)", "Back (-Z)", "Left (-X)", "Right (+X)", "Top (+Y)", "Bottom (-Y)"};
    ImGui::Combo("Side", &selectedSideIndex, sideNames, 6);
    
    if (selectedAssetIndex >= 0 && selectedAssetIndex < static_cast<int>(loadedAssets.size())) {
        int currentBlockId = loadedAssets[selectedAssetIndex].id;
        BlockUtilities::BlockSide currentSide = static_cast<BlockUtilities::BlockSide>(selectedSideIndex);
        
        // Find or create constraint for this block
        auto& constraints = parameters.directionalConstraints;
        auto constraintIt = std::find_if(constraints.begin(), constraints.end(),
            [currentBlockId](const BlockUtilities::DirectionalConstraint& c) {
                return c.blockTypeId == currentBlockId;
            });
        
        if (constraintIt == constraints.end()) {
            // Create new constraint
            constraints.emplace_back(currentBlockId, loadedAssets[selectedAssetIndex].name);
            constraintIt = constraints.end() - 1;
        }
        
        // Display current allowed neighbors for this side
        ImGui::Text("Allowed neighbors on %s:", sideNames[selectedSideIndex]);
        
        auto& sideConstraints = constraintIt->allowedNeighbors[currentSide];
        
        // Checkboxes for each possible neighbor
        for (const auto& asset : loadedAssets) {
            bool isAllowed = std::find(sideConstraints.begin(), sideConstraints.end(), asset.id) != sideConstraints.end();
            
            std::string checkboxLabel = asset.name + "##" + std::to_string(asset.id);
            if (ImGui::Checkbox(checkboxLabel.c_str(), &isAllowed)) {
                if (isAllowed) {
                    if (std::find(sideConstraints.begin(), sideConstraints.end(), asset.id) == sideConstraints.end()) {
                        sideConstraints.push_back(asset.id);
                    }
                } else {
                    sideConstraints.erase(
                        std::remove(sideConstraints.begin(), sideConstraints.end(), asset.id),
                        sideConstraints.end()
                    );
                }
            }
        }
    }
}

BlockUI::~BlockUI() {
    loadedAssets.clear();
    controller = nullptr;
}