#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "BlockUI.h"
#include "../Controllers/BlockController.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include "../Core/BlockData.h"
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>

// ONLY include what we absolutely need for file dialogs
#include "../Core/Engine/Application.h"
#include "../Utils/FileDialogs.h"
#include "../Drawables/Model.h"

BlockUI::BlockUI(BlockController* controller) : controller(controller) {
    // Initialize parameters with 3D support (gridWidth, gridHeight, gridLength, cellWidth, cellHeight, cellLength, blockScale)
    parameters = {20, 10, 20, 5.0f, 5.0f, 5.0f, 1.0f};
    // Initialize generation settings with 3D support
    genSettings = {20, 10, 20, 1.0f};
}

void BlockUI::DisplayUI() {
    if (ImGui::Begin("Block World Generator")) {
        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem("Basic")) {
                DisplayBasicSettings();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Socket System")) {
                DisplaySocketEditor();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Generate World", ImVec2(200, 40)) && controller) {
            controller->Generate();
        }
        
        DisplayBlockConstraints();  // Keep this for weight/limit controls
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
}

void BlockUI::DisplaySocketEditor() {
    if (!ImGui::CollapsingHeader("Socket Editor", ImGuiTreeNodeFlags_DefaultOpen)) return;
    
    if (loadedAssets.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Load assets first to set up sockets.");
        return;
    }
    
    // Socket type names for UI
    const char* socketTypeNames[] = {
        "Empty (connects to all)", "Grass", "Stone", "Wood", "Metal",
        "Custom 1", "Custom 2", "Custom 3", "Custom 4", "Custom 5"
    };
    
    // Block selection
    std::string selectedBlockName = selectedBlockId >= 0 ? ("Block " + std::to_string(selectedBlockId)) : "Choose block...";
    if (ImGui::BeginCombo("Select Block", selectedBlockName.c_str())) {
        for (const auto& asset : loadedAssets) {
            bool isSelected = (selectedBlockId == asset.id);
            std::string label = "Block " + std::to_string(asset.id) + " (" + asset.name + ")";
            
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedBlockId = asset.id;
            }
        }
        ImGui::EndCombo();
    }
    
    if (selectedBlockId >= 0) {
        auto& socketSystem = parameters.socketSystem;
        auto& templates = socketSystem.GetBlockTemplates();
        
        // Get or create block template
        auto it = templates.find(selectedBlockId);
        if (it == templates.end()) {
            // Create default template
            BlockTemplate newTemplate(selectedBlockId);
            newTemplate.name = "Block " + std::to_string(selectedBlockId);
            socketSystem.AddBlockTemplate(newTemplate);
        }
        
        // Get the template (now it definitely exists)
        auto& currentTemplate = const_cast<BlockTemplate&>(socketSystem.GetBlockTemplates().at(selectedBlockId));
        
        ImGui::Separator();
        ImGui::Text("Editing Sockets for Block %d", selectedBlockId);
        
        const char* faceNames[] = {"+X (Right)", "-X (Left)", "+Y (Top)", "-Y (Bottom)", "+Z (Front)", "-Z (Back)"};
        
        ImGui::Separator();

        for (int face = 0; face < 6; face++) {
            ImGui::PushID(face);
            
            ImGui::Text("%s:", faceNames[face]);
            ImGui::SameLine();
            
            int currentType = static_cast<int>(currentTemplate.sockets[face].type);
            if (ImGui::Combo("##sockettype", &currentType, socketTypeNames, 10)) {
                currentTemplate.sockets[face].type = static_cast<SocketType>(currentType);
                socketSystem.GenerateRotatedVariants(); // Regenerate when changed
            }
            
            ImGui::PopID();
        }
        
        ImGui::Separator();
        if (ImGui::Button("Apply Socket Changes")) {
            auto& templates = socketSystem.GetBlockTemplates();
            auto it = templates.find(selectedBlockId);
            if (it != templates.end()) {
                const_cast<BlockTemplate&>(it->second) = currentTemplate;
            } else {
                socketSystem.AddBlockTemplate(currentTemplate);
            }
        }
    }
    
    ImGui::Separator();
    
    // EXPANDED COMPATIBILITY RULES SECTION
    if (ImGui::CollapsingHeader("Socket Compatibility Rules", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& compatibility = parameters.socketSystem.GetCompatibility();

        ImGui::SameLine();
        if (ImGui::Button("Clear All Rules")) {
            compatibility.ClearAllRules();
            std::cout << "Cleared all socket compatibility rules" << std::endl;
        }

        ImGui::Separator();
        ImGui::Text("Create Custom Rules:");

        // Rule creation interface
        static int fromSocketType = 0;
        static int toSocketType = 0;
        static bool canConnect = true;

        ImGui::Text("From Socket:");
        ImGui::SameLine();
        ImGui::Combo("##fromSocket", &fromSocketType, socketTypeNames, 10);

        ImGui::Text("To Socket:");
        ImGui::SameLine();
        ImGui::Combo("##toSocket", &toSocketType, socketTypeNames, 10);

        ImGui::Checkbox("Can Connect", &canConnect);

        if (ImGui::Button("Add Rule")) {
            SocketType from = static_cast<SocketType>(fromSocketType);
            SocketType to = static_cast<SocketType>(toSocketType);
            compatibility.AddRule(from, to, canConnect);
            std::cout << "Added rule: " << socketTypeNames[fromSocketType]
                      << (canConnect ? " CAN " : " CANNOT ")
                      << "connect to " << socketTypeNames[toSocketType] << std::endl;
        }

        ImGui::Separator();
        ImGui::Text("Current Rules:");

        // Display current compatibility matrix
        if (ImGui::BeginTable("CompatibilityMatrix", 11, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            // Header row
            ImGui::TableSetupColumn("From \\ To");
            for (int i = 0; i < 10; i++) {
                ImGui::TableSetupColumn(socketTypeNames[i]);
            }
            ImGui::TableHeadersRow();

            // Data rows
            for (int fromType = 0; fromType < 10; fromType++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", socketTypeNames[fromType]);

                for (int toType = 0; toType < 10; toType++) {
                    ImGui::TableNextColumn();

                    SocketType from = static_cast<SocketType>(fromType);
                    SocketType to = static_cast<SocketType>(toType);
                    bool compatible = compatibility.CanConnect(from, to);

                    // Color-coded display
                    if (compatible) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓");
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗");
                    }

                    // Click to toggle
                    if (ImGui::IsItemClicked()) {
                        compatibility.AddRule(from, to, !compatible);
                        std::cout << "Toggled rule: " << socketTypeNames[fromType]
                                  << (!compatible ? " CAN " : " CANNOT ")
                                  << "connect to " << socketTypeNames[toType] << std::endl;
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::Text("Click on ✓/✗ to toggle connections");

        // Quick rule presets
        ImGui::Separator();
        ImGui::Text("Quick Presets:");

        if (ImGui::Button("All Connect")) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), true);
                }
            }
            std::cout << "Set all socket types to connect to each other" << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("None Connect")) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), false);
                }
            }
            std::cout << "Set no socket types to connect" << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("Same Only")) {
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    bool canConnect = (i == j || i == 0 || j == 0); // Same type OR Empty socket
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), canConnect);
                }
            }
            std::cout << "Set socket types to only connect to same type (+ Empty connects to all)" << std::endl;
        }
    }
}

void BlockUI::AddAsset(const AssetInfo& asset) {
    loadedAssets.push_back(asset);
}

void BlockUI::RemoveAsset(int id) {
    loadedAssets.erase(
        std::remove_if(loadedAssets.begin(), loadedAssets.end(),
            [id](const AssetInfo& asset) { return asset.id == id; }),
        loadedAssets.end()
    );
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
    
    // Initialize count, weight, and limit for this new asset
    parameters.generationSettings.currentBlockCounts[newAsset.id] = 0;
    parameters.generationSettings.blockWeights[newAsset.id] = parameters.generationSettings.defaultWeight;
    parameters.generationSettings.maxBlockCounts[newAsset.id] = -1; // Set to UNLIMITED by default
    
    std::cout << "Loaded block " << newAsset.id << " - initialized with unlimited count" << std::endl;
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

void BlockUI::DisplayBlockConstraints() {
    if (ImGui::CollapsingHeader("Block Generation Constraints", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& settings = parameters.generationSettings;
        
        ImGui::Checkbox("Enforce Block Limits", &settings.enforceBlockLimits);
        ImGui::SameLine();
        if (ImGui::Button("?")) {
            ImGui::SetTooltip("When enabled, blocks will stop generating once they reach their limit");
        }
        
        ImGui::Checkbox("Use Weighted Selection", &settings.useWeightedSelection);
        ImGui::SameLine();
        if (ImGui::Button("??")) {
            ImGui::SetTooltip("When enabled, blocks with higher weights appear more frequently");
        }
        
        ImGui::SliderFloat("Default Weight", &settings.defaultWeight, 0.0f, 1.0f);
        
        ImGui::Separator();
        
        // Show total cells for reference
        int totalCells = parameters.gridWidth * parameters.gridHeight * parameters.gridLength;
        ImGui::Text("Total Grid Cells: %d", totalCells);
        
        // Display current counts and limits for each block type
        if (ImGui::BeginTable("BlockConstraints", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Block");
            ImGui::TableSetupColumn("Weight");
            ImGui::TableSetupColumn("Count");
            ImGui::TableSetupColumn("Unlimited");
            ImGui::TableHeadersRow();
            
            auto& assets = GetLoadedAssets();
            for (auto& asset : assets) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                ImGui::Text("%s", GetFileName(asset.blockPath).c_str());
                
                ImGui::TableNextColumn();
                float weight = settings.blockWeights[asset.id];
                if (ImGui::SliderFloat(("##weight" + std::to_string(asset.id)).c_str(), &weight, 0.0f, 1.0f, "%.2f")) {
                    settings.blockWeights[asset.id] = weight;
                    std::cout << "Updated weight for block " << asset.id << " to " << weight << std::endl;
                }
                
                ImGui::TableNextColumn();
                // Make sure the count exists
                if (settings.currentBlockCounts.find(asset.id) == settings.currentBlockCounts.end()) {
                    settings.currentBlockCounts[asset.id] = 0;
                }

                bool unlimited = (settings.maxBlockCounts[asset.id] == -1);
                if (unlimited) {
                    ImGui::Text("Unlimited");
                } else {
                    int currentCount = settings.currentBlockCounts[asset.id];
                    
                    // Show count with + and - buttons
                    if (ImGui::SmallButton(("-##" + std::to_string(asset.id)).c_str())) {
                        if (settings.currentBlockCounts[asset.id] > 0) {
                            settings.currentBlockCounts[asset.id]--;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Text("%d", currentCount);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(("+##" + std::to_string(asset.id)).c_str())) {
                        settings.currentBlockCounts[asset.id]++;
                        // Also update the limit to match the desired count
                        if (settings.maxBlockCounts[asset.id] != -1) {
                            settings.maxBlockCounts[asset.id] = settings.currentBlockCounts[asset.id];
                        }
                        std::cout << "Incremented block " << asset.id << " count to " 
                                  << settings.currentBlockCounts[asset.id] << std::endl;
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::Checkbox(("##unlimited" + std::to_string(asset.id)).c_str(), &unlimited)) {
                    settings.maxBlockCounts[asset.id] = unlimited ? -1 : totalCells / 4;
                }
            }
            
            // Void cells row
            if (parameters.enableVoidCells) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Void Cells");
                
                ImGui::TableNextColumn();
                float voidWeight = settings.blockWeights[BlockUtilities::VOID_BLOCK_ID];
                if (ImGui::SliderFloat("##voidweight", &voidWeight, 0.0f, 1.0f, "%.2f")) {
                    settings.blockWeights[BlockUtilities::VOID_BLOCK_ID] = voidWeight;
                }
                
                ImGui::TableNextColumn();
                bool voidUnlimited = (settings.maxBlockCounts[BlockUtilities::VOID_BLOCK_ID] == -1);
                if (voidUnlimited) {
                    ImGui::Text("Unlimited");
                } else {
                    int voidCount = settings.currentBlockCounts[BlockUtilities::VOID_BLOCK_ID];

                    if (ImGui::SmallButton("-##void")) {
                        if (settings.currentBlockCounts[BlockUtilities::VOID_BLOCK_ID] > 0) {
                            settings.currentBlockCounts[BlockUtilities::VOID_BLOCK_ID]--;
                        }
                    }
                    ImGui::SameLine();
                    ImGui::Text("%d", voidCount);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("+##void")) {
                        settings.currentBlockCounts[BlockUtilities::VOID_BLOCK_ID]++;
                    }
                }

                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##voidunlimited", &voidUnlimited)) {
                    settings.maxBlockCounts[BlockUtilities::VOID_BLOCK_ID] = voidUnlimited ? -1 : totalCells;
                }
            }
            
            ImGui::EndTable();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Reset All Counts")) {
            for (auto& [blockId, count] : settings.currentBlockCounts) {
                count = 0;
            }
            std::cout << "Reset all block counts" << std::endl;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Equal Weights")) {
            for (auto& [blockId, weight] : settings.blockWeights) {
                weight = 0.5f;
            }
            std::cout << "Set all weights to 0.5" << std::endl;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Remove All Limits")) {
            for (auto& [blockId, limit] : settings.maxBlockCounts) {
                limit = -1; // Unlimited
            }
            std::cout << "Removed all block limits" << std::endl;
        }
        
        // Debug weight information
        if (ImGui::CollapsingHeader("Debug Info")) {
            ImGui::Text("Weights Status:");
            for (const auto& [blockId, weight] : settings.blockWeights) {
                ImGui::Text("Block %d: Weight=%.2f, Count=%d, Limit=%d", 
                           blockId, weight, 
                           settings.currentBlockCounts[blockId],
                           settings.maxBlockCounts[blockId]);
            }
        }
    }
}