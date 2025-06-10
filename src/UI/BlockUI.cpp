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

// ONLY include what we absolutely need for file dialogs
#include "../Core/Engine/Application.h"
#include "../Utils/FileDialogs.h"

BlockUI::BlockUI(IBlockUIController* controller) : controller(controller) {
    parameters.width = 100;
    parameters.length = 100;
    parameters.cellSize = 1;
    parameters.numCellsWidth = static_cast<unsigned int>(parameters.width / parameters.cellSize);
    parameters.numCellsLength = static_cast<unsigned int>(parameters.length / parameters.cellSize);
    parameters.halfWidth = parameters.width / 2.0f;
    parameters.halfLength = parameters.length / 2.0f;
    parameters.stepX = parameters.width / (parameters.numCellsWidth - 1);
    parameters.stepZ = parameters.length / (parameters.numCellsLength - 1);

    // Initialize new parameters
    parameters.blockScale = 1.0f;
}

void BlockUI::DisplayUI() {
    ImGui::Begin("Block Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
    
    DisplayGridSettings();
    ImGui::Separator();
    DisplayAssetManagement();
    ImGui::Separator();
    DisplayConstraints();
    ImGui::Separator();
    DisplayConstraintManagement();  // Add this new section
    
    if (ImGui::Button("Generate", ImVec2(200, 40))) {
        if (controller) {
            controller->Generate();
        }
    }

    ImGui::End();
}

void BlockUI::DisplayGridSettings() {
    ImGui::Text("Grid Settings");
    
    float oldWidth = parameters.width;
    float oldLength = parameters.length;
    int oldCellSize = parameters.cellSize;
    
    ImGui::DragFloat("Width", &parameters.width, 0.1f, 10, 500);
    ImGui::DragFloat("Length", &parameters.length, 0.1f, 10, 500);
    ImGui::DragInt("Cell Size", &parameters.cellSize, 0.1f, 1, 50);  // Increased max to 50
    
    if (parameters.width != oldWidth || parameters.length != oldLength || parameters.cellSize != oldCellSize) {
        parameters.numCellsWidth = static_cast<unsigned int>(parameters.width / parameters.cellSize);
        parameters.numCellsLength = static_cast<unsigned int>(parameters.length / parameters.cellSize);
        parameters.halfWidth = parameters.width / 2.0f;
        parameters.halfLength = parameters.length / 2.0f;
    }
    
    ImGui::Text("Grid Info:");
    ImGui::Text("Cells: %u x %u = %u total", parameters.numCellsWidth, parameters.numCellsLength, 
                parameters.numCellsWidth * parameters.numCellsLength);
    ImGui::Text("Cell size: %d units", parameters.cellSize);
    ImGui::Text("World bounds: %.1f x %.1f", parameters.width, parameters.length);
    
    // DEBUG: Show coordinate ranges
    ImGui::Separator();
    ImGui::Text("Debug Info:");
    ImGui::Text("X range: %.1f to %.1f", -parameters.halfWidth, parameters.halfWidth);
    ImGui::Text("Z range: %.1f to %.1f", -parameters.halfLength, parameters.halfLength);
}

void BlockUI::DisplayConstraints() {
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

void BlockUI::DisplayConstraintManagement() {
    ImGui::Text("Block Constraints");
    
    // Add constraint button
    if (ImGui::Button("Add Constraint", ImVec2(150, 0))) {
        AddNewConstraint();
    }
    
    // Display existing constraints
    if (!parameters.blockConstraints.empty()) {
        ImGui::Separator();
        ImGui::Text("Current Constraints:");
        
        for (size_t i = 0; i < parameters.blockConstraints.size(); i++) {
            auto& constraint = parameters.blockConstraints[i];
            
            ImGui::PushID(static_cast<int>(i));
            
            // Constraint header
            ImGui::Text("Block: %s (ID: %d)", constraint.blockTypeName.c_str(), constraint.blockTypeId);
            
            // Dropdown for block type selection
            std::string comboLabel = "Block Type##" + std::to_string(i);
            if (ImGui::BeginCombo(comboLabel.c_str(), constraint.blockTypeName.c_str())) {
                for (const auto& asset : loadedAssets) {
                    bool isSelected = (constraint.blockTypeId == asset.id);
                    std::string itemName = asset.name + " (ID: " + std::to_string(asset.id) + ")";
                    
                    if (ImGui::Selectable(itemName.c_str(), isSelected)) {
                        constraint.blockTypeId = asset.id;
                        constraint.blockTypeName = asset.name;
                    }
                    
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            
            // Allowed neighbors section
            ImGui::Text("Allowed Neighbors:");
            ImGui::Indent();
            
            // Add neighbor button
            if (ImGui::Button(("Add Neighbor##" + std::to_string(i)).c_str())) {
                // Add a dropdown to select neighbors
                ImGui::OpenPopup(("SelectNeighbor##" + std::to_string(i)).c_str());
            }
            
            // Neighbor selection popup
            if (ImGui::BeginPopup(("SelectNeighbor##" + std::to_string(i)).c_str())) {
                ImGui::Text("Select allowed neighbor:");
                ImGui::Separator();
                
                for (const auto& asset : loadedAssets) {
                    // Don't allow self-reference and avoid duplicates
                    bool alreadyAdded = std::find(constraint.allowedNeighbors.begin(), 
                                                constraint.allowedNeighbors.end(), 
                                                asset.id) != constraint.allowedNeighbors.end();
                    
                    if (asset.id != constraint.blockTypeId && !alreadyAdded) {
                        std::string itemName = asset.name + " (ID: " + std::to_string(asset.id) + ")";
                        if (ImGui::Selectable(itemName.c_str())) {
                            constraint.allowedNeighbors.push_back(asset.id);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndPopup();
            }
            
            // Display current neighbors with remove buttons
            for (size_t j = 0; j < constraint.allowedNeighbors.size(); j++) {
                int neighborId = constraint.allowedNeighbors[j];
                
                // Find the asset name for this ID
                std::string neighborName = "Unknown";
                for (const auto& asset : loadedAssets) {
                    if (asset.id == neighborId) {
                        neighborName = asset.name;
                        break;
                    }
                }
                
                ImGui::Text("- %s (ID: %d)", neighborName.c_str(), neighborId);
                ImGui::SameLine();
                
                if (ImGui::SmallButton(("Remove##neighbor_" + std::to_string(i) + "_" + std::to_string(j)).c_str())) {
                    constraint.allowedNeighbors.erase(constraint.allowedNeighbors.begin() + j);
                    break; // Break to avoid iterator invalidation
                }
            }
            
            ImGui::Unindent();
            
            // Remove constraint button
            if (ImGui::Button(("Remove Constraint##" + std::to_string(i)).c_str())) {
                RemoveConstraint(static_cast<int>(i));
                ImGui::PopID();
                break; // Break to avoid iterator invalidation
            }
            
            ImGui::PopID();
            ImGui::Separator();
        }
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No constraints defined.");
    }
}

void BlockUI::AddNewConstraint() {
    if (loadedAssets.empty()) {
        // Could show an error popup here
        return;
    }
    
    // Create a new constraint with the first available asset
    BlockUtilities::BlockConstraints newConstraint;
    newConstraint.blockTypeId = loadedAssets[0].id;
    newConstraint.blockTypeName = loadedAssets[0].name;
    
    parameters.blockConstraints.push_back(newConstraint);
}

void BlockUI::RemoveConstraint(int index) {
    if (index >= 0 && index < static_cast<int>(parameters.blockConstraints.size())) {
        parameters.blockConstraints.erase(parameters.blockConstraints.begin() + index);
    }
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
    // REMOVE: newAsset.texture and texturePath handling
    loadedAssets.push_back(newAsset);
}

void BlockUI::OnModelLoadError(const std::string& error) {
    lastError = error;
    showModelError = true;
}

void BlockUI::DisplayAssetManagement() {
    ImGui::Text("Asset Management");
    
    // REMOVE the texture button - only keep model loading
    if (ImGui::Button("Load Model", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        OpenModelFileDialog();
    }
    
    // Error popups - REMOVE texture error popup
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
        // UPDATED: Remove texture column
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

BlockUI::~BlockUI() {
    loadedAssets.clear();
    controller = nullptr;
}