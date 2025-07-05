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

    // Try to load default castle assets (optional - won't crash if files don't exist)
    LoadDefaultCastleAssets();

    // Setup socket configurations for each block type (will work even if no models are loaded)
    SetupDefaultSocketConfigurations();


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
        
        // Add Clear All button if there are assets
        if (!loadedAssets.empty()) {
            if (ImGui::Button("Clear All Assets", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                ImGui::OpenPopup("Clear All Assets?");
            }
            
            // Confirmation popup for clearing all
            if (ImGui::BeginPopupModal("Clear All Assets?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("Are you sure you want to remove all loaded assets?");
                ImGui::Text("This action cannot be undone.");
                ImGui::Separator();
                if (ImGui::Button("Yes, Clear All", ImVec2(120, 0))) {
                    // Clear all assets and related data
                    loadedAssets.clear();
                    auto& settings = parameters.generationSettings;
                    settings.currentBlockCounts.clear();
                    settings.blockWeights.clear();
                    settings.maxBlockCounts.clear();
                    settings.minBlockCounts.clear();
                    settings.cornerBlockIds.clear();
                    
                    // Reset generation flags
                    settings.enforceBlockLimits = true;
                    settings.useWeightedSelection = true;
                    settings.defaultWeight = 0.5f;
                    settings.isGridMaskEnabled = false;
                    
                    // Reset dimension detection
                    parameters.dimensionsDetected = false;
                    
                    // Reset UI state
                    selectedBlockId = -1;
                    rotationRequested = false;
                    assetToRemoveId = -1;
                    
                    // Clear socket system templates and reset compatibility rules
                    parameters.socketSystem.Initialize(); // This now properly clears all templates and resets compatibility

                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
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
            assetToRemoveId = -1;
            
            for (const auto& asset : loadedAssets) {
                ImGui::PushID(asset.id);
                
                // Display asset info
                ImGui::Text("ID %d: %s", asset.id, GetFileName(asset.blockPath).c_str());
                ImGui::SameLine();
                
                // Remove button
                if (ImGui::Button("Remove")) {
                    auto& settings = parameters.generationSettings;
                    settings.currentBlockCounts.erase(asset.id);
                    settings.blockWeights.erase(asset.id);
                    settings.maxBlockCounts.erase(asset.id);
                    settings.minBlockCounts.erase(asset.id);
                    settings.cornerBlockIds.erase(asset.id);
                    
                    parameters.socketSystem.RemoveBlockTemplate(asset.id);
                    
                    if (selectedBlockId == asset.id) {
                        selectedBlockId = -1;
                    }
                    
                    RemoveAsset(asset.id);

                    ImGui::PopID();
                    break;
                }
                
                ImGui::PopID();
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
                
                auto& templates = socketSystem.GetBlockTemplates();
                auto it = templates.find(selectedBlockId);
                if (it != templates.end()) {
                    const_cast<BlockTemplate&>(it->second) = currentTemplate;
                } else {
                    socketSystem.AddBlockTemplate(currentTemplate);
                }
                
                socketSystem.GenerateRotatedVariants(); // Regenerate when changed
            }
            
            ImGui::PopID();
        }
        
        ImGui::Separator();
    }
    
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Socket Compatibility Rules", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& compatibility = parameters.socketSystem.GetCompatibility();

        ImGui::Separator();
        if (ImGui::Button("Clear All Rules")) {
            compatibility.ClearAllRules();
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
        }

        ImGui::SameLine();
        if (ImGui::Button("None Connect")) {
            for (int i = 0; i < 11; i++) {
                for (int j = 0; j < 11; j++) {
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), false);
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Same Only")) {
            for (int i = 0; i < 11; i++) {
                for (int j = 0; j < 11; j++) {
                    bool canConnect = (i == j || i == 0 || j == 0); // Same type OR Empty socket
                    compatibility.AddRule(static_cast<SocketType>(i), static_cast<SocketType>(j), canConnect);
                }
            }
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
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Equal Weights")) {
            for (auto& [blockId, weight] : settings.blockWeights) {
                weight = 0.5f;
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Remove All Limits")) {
            for (auto& [blockId, limit] : settings.maxBlockCounts) {
                limit = -1; // Unlimited
            }
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

void BlockUI::SetupDefaultSocketConfigurations() {
    auto& socketSystem = parameters.socketSystem;
    
    // Clear existing templates first
    socketSystem.Initialize();
    
    // Define socket configurations for each expected block ID
    // These will only be applied if the corresponding asset was loaded
    std::map<int, std::function<void()>> socketConfigurations = {
        {0, [&]() {
            BlockTemplate tower_base(0);
            tower_base.name = "Tower Square Base";
            tower_base.sockets[0] = {SocketType::CUSTOM_5};
            tower_base.sockets[1] = {SocketType::CUSTOM_5};
            tower_base.sockets[2] = {SocketType::CUSTOM_1};
            tower_base.sockets[3] = {SocketType::CUSTOM_5};
            tower_base.sockets[4] = {SocketType::STONE};
            tower_base.sockets[5] = {SocketType::STONE};
            socketSystem.AddBlockTemplate(tower_base);
        }},
        
        {1, [&]() {
            BlockTemplate tower_arch(1);
            tower_arch.name = "Tower Square Arch";
            tower_arch.sockets[0] = {SocketType::CUSTOM_5};
            tower_arch.sockets[1] = {SocketType::CUSTOM_5};
            tower_arch.sockets[2] = {SocketType::CUSTOM_3};
            tower_arch.sockets[3] = {SocketType::CUSTOM_4};
            tower_arch.sockets[4] = {SocketType::CUSTOM_5};
            tower_arch.sockets[5] = {SocketType::CUSTOM_5};
            socketSystem.AddBlockTemplate(tower_arch);
        }},
        
        {2, [&]() {
            BlockTemplate wall(2);
            wall.name = "Wall";
            wall.sockets[0] = {SocketType::CUSTOM_5};
            wall.sockets[1] = {SocketType::CUSTOM_5};
            wall.sockets[2] = {SocketType::CUSTOM_5};
            wall.sockets[3] = {SocketType::CUSTOM_5};
            wall.sockets[4] = {SocketType::STONE};
            wall.sockets[5] = {SocketType::STONE};
            socketSystem.AddBlockTemplate(wall);
        }},
        
        {3, [&]() {
            BlockTemplate wall_gate(3);
            wall_gate.name = "Wall Narrow Gate";
            wall_gate.sockets[0] = {SocketType::CUSTOM_5};
            wall_gate.sockets[1] = {SocketType::CUSTOM_5};
            wall_gate.sockets[2] = {SocketType::CUSTOM_5};
            wall_gate.sockets[3] = {SocketType::CUSTOM_5};
            wall_gate.sockets[4] = {SocketType::WOOD};
            wall_gate.sockets[5] = {SocketType::WOOD};
            socketSystem.AddBlockTemplate(wall_gate);
        }},
        
        {4, [&]() {
            BlockTemplate wall_dup(4);
            wall_dup.name = "Wall (Duplicate)";
            wall_dup.sockets[0] = {SocketType::CUSTOM_5};
            wall_dup.sockets[1] = {SocketType::CUSTOM_5};
            wall_dup.sockets[2] = {SocketType::CUSTOM_5};
            wall_dup.sockets[3] = {SocketType::CUSTOM_5};
            wall_dup.sockets[4] = {SocketType::STONE};
            wall_dup.sockets[5] = {SocketType::WOOD};
            socketSystem.AddBlockTemplate(wall_dup);
        }},
        
        {5, [&]() {
            BlockTemplate tower_roof(5);
            tower_roof.name = "Tower Slant Roof";
            tower_roof.sockets[0] = {SocketType::CUSTOM_5};
            tower_roof.sockets[1] = {SocketType::CUSTOM_5};
            tower_roof.sockets[2] = {SocketType::CUSTOM_5};
            tower_roof.sockets[3] = {SocketType::CUSTOM_3};
            tower_roof.sockets[4] = {SocketType::CUSTOM_5};
            tower_roof.sockets[5] = {SocketType::CUSTOM_5};
            socketSystem.AddBlockTemplate(tower_roof);
        }},
        
        {6, [&]() {
            BlockTemplate wall_dup2(6);
            wall_dup2.name = "Wall (Duplicate 2)";
            wall_dup2.sockets[0] = {SocketType::CUSTOM_5};
            wall_dup2.sockets[1] = {SocketType::CUSTOM_5};
            wall_dup2.sockets[2] = {SocketType::CUSTOM_4};
            wall_dup2.sockets[3] = {SocketType::CUSTOM_5};
            wall_dup2.sockets[4] = {SocketType::STONE};
            wall_dup2.sockets[5] = {SocketType::STONE};
            socketSystem.AddBlockTemplate(wall_dup2);
        }},
        
        {7, [&]() {
            BlockTemplate wall_corner(7);
            wall_corner.name = "Wall Corner";
            wall_corner.sockets[0] = {SocketType::WALL};
            wall_corner.sockets[1] = {SocketType::STONE};
            wall_corner.sockets[2] = {SocketType::CUSTOM_5};
            wall_corner.sockets[3] = {SocketType::CUSTOM_5};
            wall_corner.sockets[4] = {SocketType::STONE};
            wall_corner.sockets[5] = {SocketType::WALL};
            socketSystem.AddBlockTemplate(wall_corner);
        }},
        
        {8, [&]() {
            BlockTemplate wall_corner_tower(8);
            wall_corner_tower.name = "Wall Corner Half Tower";
            wall_corner_tower.sockets[0] = {SocketType::WALL};
            wall_corner_tower.sockets[1] = {SocketType::STONE};
            wall_corner_tower.sockets[2] = {SocketType::CUSTOM_5};
            wall_corner_tower.sockets[3] = {SocketType::CUSTOM_5};
            wall_corner_tower.sockets[4] = {SocketType::WALL};
            wall_corner_tower.sockets[5] = {SocketType::STONE};
            socketSystem.AddBlockTemplate(wall_corner_tower);
        }},
        
        {9, [&]() {
            BlockTemplate tower_mid(9);
            tower_mid.name = "Tower Square Mid";
            tower_mid.sockets[0] = {SocketType::CUSTOM_5};
            tower_mid.sockets[1] = {SocketType::CUSTOM_5};
            tower_mid.sockets[2] = {SocketType::CUSTOM_2};
            tower_mid.sockets[3] = {SocketType::CUSTOM_1};
            tower_mid.sockets[4] = {SocketType::CUSTOM_5};
            tower_mid.sockets[5] = {SocketType::CUSTOM_5};
            socketSystem.AddBlockTemplate(tower_mid);
        }},
        
        {10, [&]() {
            BlockTemplate tower_mid_windows(10);
            tower_mid_windows.name = "Tower Square Mid Windows";
            tower_mid_windows.sockets[0] = {SocketType::CUSTOM_5};
            tower_mid_windows.sockets[1] = {SocketType::CUSTOM_5};
            tower_mid_windows.sockets[2] = {SocketType::CUSTOM_3};
            tower_mid_windows.sockets[3] = {SocketType::CUSTOM_2};
            tower_mid_windows.sockets[4] = {SocketType::CUSTOM_5};
            tower_mid_windows.sockets[5] = {SocketType::CUSTOM_5};
            socketSystem.AddBlockTemplate(tower_mid_windows);
        }},
        
        {11, [&]() {
            BlockTemplate tower_top_roof(11);
            tower_top_roof.name = "Tower Square Top Roof High";
            tower_top_roof.sockets[0] = {SocketType::CUSTOM_5};
            tower_top_roof.sockets[1] = {SocketType::CUSTOM_5};
            tower_top_roof.sockets[2] = {SocketType::CUSTOM_5};
            tower_top_roof.sockets[3] = {SocketType::CUSTOM_3};
            tower_top_roof.sockets[4] = {SocketType::CUSTOM_5};
            tower_top_roof.sockets[5] = {SocketType::CUSTOM_5};
            socketSystem.AddBlockTemplate(tower_top_roof);
        }},
        
        {12, [&]() {
            BlockTemplate tower_mid_windows_dup(12);
            tower_mid_windows_dup.name = "Tower Square Mid Windows";
            tower_mid_windows_dup.sockets[0] = {SocketType::CUSTOM_5};
            tower_mid_windows_dup.sockets[1] = {SocketType::CUSTOM_5};
            tower_mid_windows_dup.sockets[2] = {SocketType::CUSTOM_3};
            tower_mid_windows_dup.sockets[3] = {SocketType::CUSTOM_3};
            tower_mid_windows_dup.sockets[4] = {SocketType::CUSTOM_5};
            tower_mid_windows_dup.sockets[5] = {SocketType::CUSTOM_5};
            socketSystem.AddBlockTemplate(tower_mid_windows_dup);
        }}
    };
    
    // Apply socket configurations only for loaded assets
    int configuredCount = 0;
    std::set<int> cornerBlockIds;
    
    for (const auto& asset : loadedAssets) {
        auto configIt = socketConfigurations.find(asset.id);
        if (configIt != socketConfigurations.end()) {
            configIt->second(); // Execute the configuration function
            configuredCount++;
            
        } else {
            std::cout << "! No socket configuration defined for block " << asset.id << " - using defaults" << std::endl;
        }
    }

    // Set up socket compatibility rules
    auto& compatibility = socketSystem.GetCompatibility();

    // Clear all rules first
    compatibility.ClearAllRules();

    // Basic connection rules - only specific socket types can connect to each other:
    compatibility.AddRule(SocketType::EMPTY, SocketType::EMPTY, true);
    compatibility.AddRule(SocketType::STONE, SocketType::STONE, true);
    compatibility.AddRule(SocketType::WOOD, SocketType::WOOD, true);
    compatibility.AddRule(SocketType::WALL, SocketType::EMPTY, true);  // WALL can only connect to empty space
    compatibility.AddRule(SocketType::CUSTOM_1, SocketType::CUSTOM_1, true);
    compatibility.AddRule(SocketType::CUSTOM_2, SocketType::CUSTOM_2, true);
    compatibility.AddRule(SocketType::CUSTOM_3, SocketType::CUSTOM_3, true);
    compatibility.AddRule(SocketType::CUSTOM_4, SocketType::CUSTOM_4, true);
    
    // Cross-compatibility rules for architectural coherence
    compatibility.AddRule(SocketType::STONE, SocketType::WOOD, true);    // Stone can connect to wood
    compatibility.AddRule(SocketType::CUSTOM_1, SocketType::CUSTOM_3, true); // Tower base to tower mid windows
    compatibility.AddRule(SocketType::CUSTOM_2, SocketType::CUSTOM_3, true); // Tower mid to tower mid windows

    // Generate rotated variants for all templates
    socketSystem.GenerateRotatedVariants();
    

     for (const auto& asset : loadedAssets) {
            auto& settings = parameters.generationSettings;
            
            // Set default weights based on block type
            switch (asset.id) {
                case 0: settings.blockWeights[asset.id] = 0.08f; break; // Tower base
                case 1: settings.blockWeights[asset.id] = 0.05f; break; // Tower arch
                case 2: settings.blockWeights[asset.id] = 0.93f; break; // Wall (main)
                case 3: settings.blockWeights[asset.id] = 0.50f; break; // Gate
                case 4: settings.blockWeights[asset.id] = 0.50f; break; // Wall dup
                case 5: settings.blockWeights[asset.id] = 0.90f; break; // Tower roof
                case 6: settings.blockWeights[asset.id] = 0.50f; break; // Wall dup 2
                default: settings.blockWeights[asset.id] = 0.50f; break; // Others
            }
            
            // Set default limits
            settings.maxBlockCounts[asset.id] = -1; // Unlimited by default
            settings.minBlockCounts[asset.id] = 0;  // No minimum by default
            
            // Special cases
            if (asset.id == 3) { // Gate - only want 1, and require at least 1
                settings.maxBlockCounts[asset.id] = 1;
                settings.minBlockCounts[asset.id] = 1;
            }
            if (asset.id == 4) { // Wall variant - limit to 2, require at least 1
                settings.maxBlockCounts[asset.id] = 2;
                settings.minBlockCounts[asset.id] = 1;
            }
        }
        
    // Only enable castle system and set corner blocks if we have corner blocks loaded
    if (!cornerBlockIds.empty()) {
        parameters.generationSettings.isGridMaskEnabled = true;
        parameters.generationSettings.cornerBlockIds = cornerBlockIds;

       
    } else {
        std::cout << "No corner blocks loaded - castle system will remain disabled" << std::endl;
    }
}

void BlockUI::LoadDefaultCastleAssets() {
    if (!controller) {
        std::cout << "No controller available - skipping default asset loading" << std::endl;
        return;
    }
    
        // Get the path to this source file and construct project root path
    std::string currentFilePath = __FILE__;
    
    // Navigate up from src/UI/BlockUI.cpp to project root
    // Remove the filename first
    size_t lastSlash = currentFilePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        currentFilePath = currentFilePath.substr(0, lastSlash); // Remove BlockUI.cpp
    }
    
    // Go up from UI directory
    lastSlash = currentFilePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        currentFilePath = currentFilePath.substr(0, lastSlash); // Remove UI
    }
    
    // Go up from src directory  
    lastSlash = currentFilePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        currentFilePath = currentFilePath.substr(0, lastSlash); // Remove src
    }
    
    // Convert backslashes to forward slashes for consistency
    std::replace(currentFilePath.begin(), currentFilePath.end(), '\\', '/');
    
    std::string basePath = currentFilePath + "/";

    std::string assetPaths[] = {
        basePath + "Models/Castle/tower-square-base.fbx",
        basePath + "Models/Castle/tower-square-arch.fbx",
        basePath + "Models/Castle/wall.fbx",
        basePath + "Models/Castle/wall-narrow-gate.fbx",
        basePath + "Models/Castle/wall.fbx", // Duplicate
        basePath + "Models/Castle/tower-slant-roof.fbx",
        basePath + "Models/Castle/wall.fbx", // Duplicate
        basePath + "Models/Castle/wall-corner.fbx",
        basePath + "Models/Castle/wall-corner-half-tower.fbx",
        basePath + "Models/Castle/tower-square-mid.fbx",
        basePath + "Models/Castle/tower-square-mid-windows.fbx",
        basePath + "Models/Castle/tower-square-top-roof-high.fbx",
        basePath + "Models/Castle/tower-square-mid-windows.fbx" // Duplicate
    };

    for (int i = 0; i < sizeof(assetPaths) / sizeof(assetPaths[0]); ++i) {
        std::shared_ptr<Model> model = std::make_shared<Model>(static_cast<const char*>(assetPaths[i].c_str()));
        
        AssetInfo newAsset = {
            i,
            "Castle_" + std::to_string(i),
            assetPaths[i],
            model
        };
        
        loadedAssets.push_back(newAsset);
        
        // Initialize generation settings for this asset
        parameters.generationSettings.currentBlockCounts[newAsset.id] = 0;
        parameters.generationSettings.blockWeights[newAsset.id] = parameters.generationSettings.defaultWeight;
        parameters.generationSettings.maxBlockCounts[newAsset.id] = -1;
            
    }
    
}

