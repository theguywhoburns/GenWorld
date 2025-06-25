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
#include <iostream>
#include <functional>
#include <thread>

// ONLY include what we absolutely need for file dialogs
#include "../Core/Engine/Application.h"
#include "../Utils/FileDialogs.h"
#include "../Drawables/Model.h"

BlockUI::BlockUI(IBlockUIController* controller) : controller(controller) {
    // Initialize parameters with 3D support (gridWidth, gridHeight, gridLength, cellWidth, cellHeight, cellLength, blockScale)
    parameters = {20, 10, 20, 5.0f, 5.0f, 5.0f, 1.0f};
    // Initialize generation settings with 3D support
    genSettings = {4, true, 20, 10, 20, 1.0f, false, 50.0f};
    resetConstraintsToDefault();
}

void BlockUI::DisplayUI() {
    if (ImGui::Begin("Block World Generator")) {
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Basic")) {
                DisplayBasicSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Constraints")) {
                DisplayConstraints();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Generate World", ImVec2(200, 40)) && controller) {
            controller->Generate();
        }
    }
    ImGui::End();
}

void BlockUI::DisplayBasicSettings() {
    // Grid Settings
    if (ImGui::CollapsingHeader("Grid Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragInt("Grid Width (blocks)", reinterpret_cast<int*>(&parameters.gridWidth), 1, 5, 100);
        ImGui::DragInt("Grid Height (blocks)", reinterpret_cast<int*>(&parameters.gridHeight), 1, 5, 50);
        ImGui::DragInt("Grid Length (blocks)", reinterpret_cast<int*>(&parameters.gridLength), 1, 5, 100);
        
        // Sync settings
        genSettings.gridWidth = parameters.gridWidth;
        genSettings.gridHeight = parameters.gridHeight;
        genSettings.gridLength = parameters.gridLength;
        
        ImGui::Text("Grid size: %u x %u x %u = %u total blocks", 
                    parameters.gridWidth, parameters.gridHeight, parameters.gridLength,
                    parameters.gridWidth * parameters.gridHeight * parameters.gridLength);
        
        // Show detected cell dimensions if available
        if (parameters.dimensionsDetected) {
            ImGui::Text("Cell size: %.2f x %.2f x %.2f units", 
                       parameters.cellWidth, parameters.cellHeight, parameters.cellLength);
            ImGui::Text("World size: %.1f x %.1f x %.1f units", 
                       parameters.worldWidth, parameters.worldHeight, parameters.worldLength);
        }
    }
    
    // Block Settings
    if (ImGui::CollapsingHeader("Block Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Block Scale", &parameters.blockScale, 0.01f, 0.1f, 5.0f);
        genSettings.blockScale = parameters.blockScale;
        
        if (ImGui::Button("Reset Scale")) {
            parameters.blockScale = genSettings.blockScale = 1.0f;
        }
        
        ImGui::Separator();
        
        // Random Rotation Settings
        ImGui::Checkbox("Enable Random Y-Axis Rotations", &genSettings.enableRandomRotations);
        if (genSettings.enableRandomRotations) {
            ImGui::TextColored(ImVec4(0.7f, 0.9f, 0.7f, 1.0f), "Blocks will be randomly rotated around Y-axis (0°, 90°, 180°, 270°)");
        }
        
        // Apply Random Rotations button (only if world is already generated)
        if (ImGui::Button("Apply Random Rotations to Existing World", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            // Call the rotation method directly through a callback or event
            OnApplyRandomRotationsRequested();
        }
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "This will apply random Y-axis rotations to all blocks in the current world");
    }
    
    // Asset Management
    if (ImGui::CollapsingHeader("Asset Management", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Load Model", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            OpenModelFileDialog();
        }
        
        // Error popup
        if (showModelError) {
            ImGui::OpenPopup("Model Load Error");
            showModelError = false;
        }
        
        if (ImGui::BeginPopupModal("Model Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load model file!\n%s", lastError.c_str());
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::Text("Loaded Assets:");
        if (loadedAssets.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No assets loaded.");
        } else {
            for (const auto& asset : loadedAssets) {
                ImGui::Text("ID %d: %s", asset.id, GetFileName(asset.blockPath).c_str());
            }
        }
    }
    
    // Generation Settings
    if (ImGui::CollapsingHeader("Generation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("Thread Count", &genSettings.threadCount, 1, 16);
        ImGui::Text("Recommended: %d threads", std::thread::hardware_concurrency());
        
        ImGui::Checkbox("Enable Constraints", &genSettings.useConstraints);
        
        ImGui::Separator();
        ImGui::Checkbox("Enable Popping Animation", &genSettings.enablePoppingAnimation);
        
        if (genSettings.enablePoppingAnimation) {
            ImGui::SliderFloat("Animation Delay (ms)", &genSettings.animationDelay, 10.0f, 500.0f, "%.1f ms");
            ImGui::Text("Blocks will appear with %.1f ms delay between each", genSettings.animationDelay);
            
            if (genSettings.threadCount > 1) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Note: Animation forces single-threaded generation");
            }
        }
    }
}

void BlockUI::DisplayConstraints() {
    if (loadedAssets.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Load assets first to set up constraints.");
        return;
    }
    
    if (!ImGui::CollapsingHeader("Block Constraint Editor", ImGuiTreeNodeFlags_DefaultOpen)) return;
    
    // Block selection
    if (ImGui::BeginCombo("Select Block", selectedBlockId >= 0 ? std::to_string(selectedBlockId).c_str() : "Choose block...")) {
        for (const auto& asset : loadedAssets) {
            bool isSelected = (selectedBlockId == asset.id);
            std::string label = "Block " + std::to_string(asset.id) + " (" + asset.name + ")";
            
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedBlockId = asset.id;
                
                // Create default constraints if they don't exist
                if (uiConstraints.find(asset.id) == uiConstraints.end()) {
                    uiConstraints[asset.id] = createDefaultConstraints(asset.id);
                }
            }
        }
        ImGui::EndCombo();
    }
    
    // Edit selected block constraints
    if (selectedBlockId >= 0 && uiConstraints.find(selectedBlockId) != uiConstraints.end()) {
        ImGui::Separator();
        ImGui::Text("Editing Block %d", selectedBlockId);
        
        auto& constraints = uiConstraints[selectedBlockId];
        
        ImGui::Separator();
        ImGui::Text("Face Constraints:");
        ImGui::Text("Select which models can connect to each face of this block:");
        
        const char* faceNames[] = {"+Z (Front)", "-Z (Back)", "+X (Right)", "-X (Left)", "+Y (Top)", "-Y (Bottom)"};
        BlockFaceConstraints* faces[] = {&constraints.posZ, &constraints.negZ, &constraints.posX, 
                                       &constraints.negX, &constraints.posY, &constraints.negY};
        
        for (int f = 0; f < 6; f++) {
            if (ImGui::TreeNode(faceNames[f])) {
                ImGui::Checkbox("Can be exposed to air", &faces[f]->canBeExposed);
                ImGui::Text("Allowed connecting models:");
                
                // Show checkboxes for each loaded asset
                for (const auto& asset : loadedAssets) {
                    bool isAllowed = std::find(faces[f]->validConnections.begin(), 
                                             faces[f]->validConnections.end(), 
                                             asset.id) != faces[f]->validConnections.end();
                    
                    std::string checkboxLabel = "Model " + std::to_string(asset.id) + " (" + asset.name + ")";
                    
                    if (ImGui::Checkbox(checkboxLabel.c_str(), &isAllowed)) {
                        updateConstraintConnection(faces[f], asset.id, isAllowed);
                    }
                }
                
                // Quick action buttons
                if (ImGui::Button("Allow All Models")) {
                    faces[f]->validConnections.clear();
                    for (const auto& asset : loadedAssets) {
                        faces[f]->validConnections.push_back(asset.id);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear All")) {
                    faces[f]->validConnections.clear();
                }
                
                // Show current state
                if (faces[f]->validConnections.empty()) {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No restrictions - can connect to any model");
                } else {
                    ImGui::Text("Restricted to %zu model(s)", faces[f]->validConnections.size());
                }
                
                ImGui::TreePop();
            }
        }
    }
    
    if (ImGui::Button("Reset All Constraints")) {
        resetConstraintsToDefault();
    }
}

BlockConstraints BlockUI::createDefaultConstraints(int blockId) {
    BlockConstraints constraints;
    constraints.blockId = blockId;
    constraints.posZ = constraints.negZ = constraints.posX = 
    constraints.negX = constraints.posY = constraints.negY = {{}, true};
    return constraints;
}

void BlockUI::updateConstraintConnection(BlockFaceConstraints* face, int assetId, bool isAllowed) {
    auto& connections = face->validConnections;
    auto it = std::find(connections.begin(), connections.end(), assetId);
    
    if (isAllowed && it == connections.end()) {
        connections.push_back(assetId);
    } else if (!isAllowed && it != connections.end()) {
        connections.erase(it);
    }
}

void BlockUI::resetConstraintsToDefault() {
    uiConstraints.clear();
    for (const auto& asset : loadedAssets) {
        uiConstraints[asset.id] = createDefaultConstraints(asset.id);
    }
}

const std::map<int, BlockConstraints>& BlockUI::GetConstraints() const {
    return uiConstraints;
}

void BlockUI::SetConstraints(const std::map<int, BlockConstraints>& constraints) {
    uiConstraints = constraints;
}

void BlockUI::AddAsset(const AssetInfo& asset) {
    loadedAssets.push_back(asset);
    resetConstraintsToDefault();
}

void BlockUI::RemoveAsset(int id) {
    loadedAssets.erase(
        std::remove_if(loadedAssets.begin(), loadedAssets.end(),
            [id](const AssetInfo& asset) { return asset.id == id; }),
        loadedAssets.end()
    );
    uiConstraints.erase(id);
}

void BlockUI::OpenModelFileDialog() {
    GLFWwindow* windowHandle = Application::GetInstance()->GetWindow()->getNativeWindow();
    
    std::string filepath = Utils::FileDialogs::OpenFile(
        "3D Model Files", 
        "Model Files (*.obj;*.fbx;*.gltf;*.glb)\0*.obj;*.fbx;*.gltf;*.glb\0All Files\0*.*\0",
        windowHandle
    );
    
    if (!filepath.empty() && controller) {
        controller->LoadModel(filepath);
    }
}

void BlockUI::OnModelLoaded(std::shared_ptr<Model> model, const std::string& filepath) {
    AssetInfo newAsset = {
        loadedAssets.empty() ? 0 : loadedAssets.back().id + 1,
        "Model_" + std::to_string(loadedAssets.size()),
        filepath,
        model
    };
    loadedAssets.push_back(newAsset);
    resetConstraintsToDefault();
}

void BlockUI::OnModelLoadError(const std::string& error) {
    lastError = error;
    showModelError = true;
}

std::string BlockUI::GetFileName(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    return lastSlash != std::string::npos ? filepath.substr(lastSlash + 1) : filepath;
}

BlockUI::~BlockUI() {
    loadedAssets.clear();
    controller = nullptr;
}

// Add this new method to BlockUI
void BlockUI::OnApplyRandomRotationsRequested() {
    // This will be handled by whoever owns the BlockGenerator
    // For now, we can store a flag or use a callback system
    rotationRequested = true;
}