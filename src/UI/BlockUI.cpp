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
#include <random>

BlockUI::BlockUI(BlockController* controller) : controller(controller) {
        parameters = BlockUtilities::BlockData(
        20,     // gridWidth
        10,     // gridHeight
        20,     // gridLength
        5.0f,   // cellWidth
        5.0f,   // cellHeight
        5.0f,   // cellLength
        1.0f,   // blockScale
        1.0f,   // gridScale
        12345  // randomSeed
    );

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
        // Center the Generate World button and make it big
        float buttonWidth = 200.0f;
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((avail - buttonWidth) * 0.5f);
        if (ImGui::Button("Generate World", ImVec2(buttonWidth, 40)) && controller) {
            controller->Generate();
        }

        DisplayBlockConstraints();  // Keep this for weight/limit controls
        DisplayCastleMakerSettings();  // Keep this for castle maker settings
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
        
        if (ImGui::Button("Reset Block Scale")) {
            parameters.blockScale = genSettings.blockScale = 1.0f;
        }
        
        ImGui::Separator();
        
    ImGui::DragFloat("Grid Scale", &parameters.gridScale, 0.01f, 0.1f, 10.0f);

    if (ImGui::Button("Reset Grid Scale")) {
        parameters.gridScale = 1.0f;
    }
        
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

    ImGui::Separator();
    ImGui::InputInt("Random Seed", reinterpret_cast<int*>(&parameters.randomSeed));

    // Center the Randomize Seed button and make it big like Generate
    float buttonWidth = 200.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX((avail - buttonWidth) * 0.5f);
    if (ImGui::Button("Randomize Seed", ImVec2(buttonWidth, 40))) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned int> dist(10000, 99999);
        parameters.randomSeed = dist(gen);
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
        "Empty (connects to all)", "Grass", "Stone", "Wood", "Metal", "Wall (exterior)",
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
            if (ImGui::Combo("##sockettype", &currentType, socketTypeNames, 11)) {
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

        ImGui::Separator();
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
        ImGui::Combo("##fromSocket", &fromSocketType, socketTypeNames, 11);

        ImGui::Text("To Socket:");
        ImGui::SameLine();
        ImGui::Combo("##toSocket", &toSocketType, socketTypeNames, 11);

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
        if (ImGui::BeginTable("CompatibilityMatrix", 12, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            // Header row
            ImGui::TableSetupColumn("From \\ To");
            for (int i = 0; i < 11; i++) {
                ImGui::TableSetupColumn(socketTypeNames[i]);
            }
            ImGui::TableHeadersRow();

            // Data rows
            for (int fromType = 0; fromType < 11; fromType++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", socketTypeNames[fromType]);

                for (int toType = 0; toType < 11; toType++) {
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
            for (int i = 0; i < 11; i++) {
                for (int j = 0; j < 11; j++) {
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), true);
                }
            }
            std::cout << "Set all socket types to connect to each other" << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("None Connect")) {
            for (int i = 0; i < 11; i++) {
                for (int j = 0; j < 11; j++) {
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), false);
                }
            }
            std::cout << "Set no socket types to connect" << std::endl;
        }

        ImGui::SameLine();
        if (ImGui::Button("Same Only")) {
            for (int i = 0; i < 11; i++) {
                for (int j = 0; j < 11; j++) {
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
        
        ImGui::SliderFloat("Default Weight", &settings.defaultWeight, 0.0f, 1.0f);
        
        ImGui::Separator();
        
        // Show total cells for reference
        int totalCells = parameters.gridWidth * parameters.gridHeight * parameters.gridLength;
        ImGui::Text("Total Grid Cells: %d", totalCells);
        
        // Display current counts and limits for each block type
        if (ImGui::BeginTable("BlockConstraints", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Block");
            ImGui::TableSetupColumn("Weight");
            ImGui::TableSetupColumn("Min");
            ImGui::TableSetupColumn("Max");
            ImGui::TableSetupColumn("Unlimited");
            ImGui::TableHeadersRow();
            
            auto& assets = GetLoadedAssets();
            for (auto& asset : assets) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                // Check if this block is assigned as a corner block
                bool isCornerBlock = settings.cornerBlockIds.count(asset.id) > 0;
                
                if (isCornerBlock) {
                    ImGui::TextColored(ImVec4(0.7f, 0.5f, 1.0f, 1.0f), "%s (Corner)", GetFileName(asset.blockPath).c_str());
                } else {
                    ImGui::Text("%s", GetFileName(asset.blockPath).c_str());
                }
                
                ImGui::TableNextColumn();
                float weight = settings.blockWeights[asset.id];
                
                if (isCornerBlock) {
                    // Disable weight control for corner blocks
                    ImGui::BeginDisabled();
                    ImGui::SliderFloat(("##weight" + std::to_string(asset.id)).c_str(), &weight, 0.0f, 1.0f, "Corner");
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("Corner blocks use special placement logic, weight is not applicable");
                    }
                } else {
                    if (ImGui::SliderFloat(("##weight" + std::to_string(asset.id)).c_str(), &weight, 0.0f, 1.0f, "%.2f")) {
                        settings.blockWeights[asset.id] = weight;
                        std::cout << "Updated weight for block " << asset.id << " to " << weight << std::endl;
                    }
                }
                
                // Ensure min/max exist
                if (settings.minBlockCounts.find(asset.id) == settings.minBlockCounts.end()) {
                    settings.minBlockCounts[asset.id] = 0;
                }
                if (settings.maxBlockCounts.find(asset.id) == settings.maxBlockCounts.end()) {
                    settings.maxBlockCounts[asset.id] = -1;
                }
    
                // Min input field
                ImGui::TableNextColumn();
                int minCount = settings.minBlockCounts[asset.id];
                
                if (isCornerBlock) {
                    // Disable min count for corner blocks
                    ImGui::BeginDisabled();
                    ImGui::Text("Corner");
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("Corner blocks are placed using special logic, min count is not applicable");
                    }
                } else {
                    char minBuf[16];
                    snprintf(minBuf, sizeof(minBuf), "%d", minCount);
                    if (ImGui::InputText(("##min" + std::to_string(asset.id)).c_str(), minBuf, sizeof(minBuf), ImGuiInputTextFlags_CharsDecimal)) {
                        int newMin = atoi(minBuf);
                        newMin = std::max(0, newMin);
                        if (settings.maxBlockCounts[asset.id] != -1 && newMin > settings.maxBlockCounts[asset.id]) {
                            newMin = settings.maxBlockCounts[asset.id];
                            snprintf(minBuf, sizeof(minBuf), "%d", newMin); // update buffer if clamped
                        }
                        settings.minBlockCounts[asset.id] = newMin;
                    }
                }
                
                // Max input field
                ImGui::TableNextColumn();
                int maxCount = settings.maxBlockCounts[asset.id];
                
                if (isCornerBlock) {
                    // Disable max count for corner blocks
                    ImGui::BeginDisabled();
                    ImGui::Text("Corner");
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("Corner blocks are placed using special logic, max count is not applicable");
                    }
                } else {
                    if (maxCount == -1) {
                        ImGui::Text("Unlimited");
                    } else {
                        char maxBuf[16];
                        snprintf(maxBuf, sizeof(maxBuf), "%d", maxCount);
                        if (ImGui::InputText(("##max" + std::to_string(asset.id)).c_str(), maxBuf, sizeof(maxBuf), ImGuiInputTextFlags_CharsDecimal)) {
                            int newMax = atoi(maxBuf);
                            newMax = std::max(settings.minBlockCounts[asset.id], newMax);
                            newMax = std::min(newMax, totalCells);
                            settings.maxBlockCounts[asset.id] = newMax;
                        }
                    }
                }

                // Unlimited checkbox
                ImGui::TableNextColumn();
                
                if (isCornerBlock) {
                    // Disable unlimited checkbox for corner blocks
                    ImGui::BeginDisabled();
                    bool dummyUnlimited = false;
                    ImGui::Checkbox(("##unlimited" + std::to_string(asset.id)).c_str(), &dummyUnlimited);
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("Corner blocks are placed using special logic, count limits are not applicable");
                    }
                } else {
                    bool unlimited = (settings.maxBlockCounts[asset.id] == -1);
                    if (ImGui::Checkbox(("##unlimited" + std::to_string(asset.id)).c_str(), &unlimited)) {
                        settings.maxBlockCounts[asset.id] = unlimited ? -1 : std::max(settings.minBlockCounts[asset.id], totalCells / 4);
                    }
                }
            }
            
            ImGui::EndTable();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Reset All Counts")) {
            for (auto& [blockId, count] : settings.minBlockCounts) {
                count = 0;
            }
            for (auto& [blockId, count] : settings.maxBlockCounts) {
                count = -1;
            }
            std::cout << "Reset all block min/max counts" << std::endl;
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
    }
}

void BlockUI::DisplayCastleMakerSettings() {
    // Sync from parameters to local UI set
    auto selectedCornerBlockIds = parameters.generationSettings.cornerBlockIds;
    bool castleSystemEnabled = parameters.generationSettings.isGridMaskEnabled;

    if (ImGui::CollapsingHeader("Castle Maker Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Castle Grid", &castleSystemEnabled);

        if (castleSystemEnabled) {
            ImGui::Text("Select valid corner pieces:");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Corner blocks must have at least 2 Wall sockets on X and Z axes");
            
            for (const auto& asset : loadedAssets) {
                bool isCorner = selectedCornerBlockIds.count(asset.id) > 0;
                std::string label = "Corner: " + GetFileName(asset.blockPath);
                
                // Check if this block is valid as a corner block
                bool isValidCorner = validateCornerBlock(asset.id);
                
                if (!isValidCorner && isCorner) {
                    // If it's selected but not valid, show warning
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "⚠");
                    ImGui::SameLine();
                }
                
                if (ImGui::Checkbox((label + "##corner" + std::to_string(asset.id)).c_str(), &isCorner)) {
                    if (isCorner) {
                        if (isValidCorner) {
                            selectedCornerBlockIds.insert(asset.id);
                        } else {
                            isCorner = false; // Reset checkbox
                            ImGui::OpenPopup(("Invalid Corner Block##" + std::to_string(asset.id)).c_str());
                        }
                    } else {
                        selectedCornerBlockIds.erase(asset.id);
                    }
                }
                
                // Show validation popup
                if (ImGui::BeginPopupModal(("Invalid Corner Block##" + std::to_string(asset.id)).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("This block cannot be used as a corner block!");
                    ImGui::Text("Corner blocks must have at least 2 Wall sockets");
                    ImGui::Text("on the X and Z axes (+X, -X, +Z, -Z faces).");
                    ImGui::Separator();
                    ImGui::Text("Please set the appropriate faces to 'Wall (exterior)'");
                    ImGui::Text("in the Socket Editor tab.");
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                
                if (!isValidCorner) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.7f, 0.3f, 0.3f, 1.0f), "(Invalid - needs 2+ Wall sockets on X/Z axes)");
                }
            }

            ImGui::Separator();
            ImGui::Text("Selected corner block IDs:");
            for (int id : selectedCornerBlockIds) {
                ImGui::SameLine();
                ImGui::Text("%d", id);
            }
        }
    }

    // Sync back to parameters
    parameters.generationSettings.cornerBlockIds = selectedCornerBlockIds;
    parameters.generationSettings.isGridMaskEnabled = castleSystemEnabled;
}

bool BlockUI::validateCornerBlock(int blockId) const {
    // Check if this block has the required Wall sockets for corner block usage
    auto& templates = parameters.socketSystem.GetBlockTemplates();
    auto templateIt = templates.find(blockId);
    
    if (templateIt == templates.end()) {
        return false; // No template found
    }
    
    const auto& blockTemplate = templateIt->second;
    
    // Count Wall sockets on X and Z axes
    // Face indices: 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
    int wallCount = 0;
    
    // Check X axis faces
    if (blockTemplate.sockets[0].type == SocketType::WALL) wallCount++; // +X
    if (blockTemplate.sockets[1].type == SocketType::WALL) wallCount++; // -X
    
    // Check Z axis faces
    if (blockTemplate.sockets[4].type == SocketType::WALL) wallCount++; // +Z
    if (blockTemplate.sockets[5].type == SocketType::WALL) wallCount++; // -Z
    
    // Must have at least 2 Wall sockets on X and Z axes
    return wallCount >= 2;
}