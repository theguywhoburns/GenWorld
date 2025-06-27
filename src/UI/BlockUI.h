#pragma once

#include "../Core/BlockData.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>

class BlockController;
class Model;

struct AssetInfo {
    int id;
    std::string name;
    std::string blockPath;
    std::shared_ptr<Model> model;
};

struct GenerationSettings {
    int threadCount = 4;
    bool useConstraints = true;
    unsigned int gridWidth = 20;
    unsigned int gridHeight = 10;
    unsigned int gridLength = 20;
    float blockScale = 1.0f;
    bool enablePoppingAnimation = false;
    float animationDelay = 50.0f;
    bool enableRandomRotations = false; // NEW: Random Y-axis rotations
};

class BlockUI {
private:
    BlockController* controller;
    std::vector<AssetInfo> loadedAssets;
    
    // Parameters and settings
    BlockUtilities::BlockData parameters;
    GenerationSettings genSettings;
    
    // Socket system selection
    int selectedBlockId = -1;
    bool rotationRequested = false;
    
    // Error handling
    bool showModelError = false;
    std::string lastError;

public:
    BlockUI(BlockController* controller);
    ~BlockUI();
    
    // Main UI display
    void DisplayUI();
    
    // Tab displays
    void DisplayBasicSettings();
    void DisplayBlockConstraints();
    
    // NEW: Socket system editor
    void DisplaySocketEditor();
    
    // Asset management
    void OnModelLoaded(std::shared_ptr<Model> model, const std::string& filepath);
    void OnModelLoadError(const std::string& error);
    void AddAsset(const AssetInfo& asset);
    void RemoveAsset(int id);
    
    // Parameters access
    const BlockUtilities::BlockData& GetParameters() const { return parameters; }
    BlockUtilities::BlockData& GetParameters() { return parameters; }
    void SetParameters(const BlockUtilities::BlockData& params) { parameters = params; }
    
    const GenerationSettings& GetGenerationSettings() const { return genSettings; }
    
    // Asset access
    const std::vector<AssetInfo>& GetLoadedAssets() const { return loadedAssets; }
    std::vector<AssetInfo>& GetLoadedAssets() { return loadedAssets; }
    
    // Animation settings
    bool IsAnimationEnabled() const { return genSettings.enablePoppingAnimation; }
    float GetAnimationDelay() const { return genSettings.animationDelay; }
    
    // NEW: Rotation settings
    bool IsRandomRotationEnabled() const { return genSettings.enableRandomRotations; }
    bool IsRotationRequested() const { return rotationRequested; }
    void ClearRotationRequest() { rotationRequested = false; }
    void OnApplyRandomRotationsRequested();

private:
    // File operations
    void OpenModelFileDialog();
    std::string GetFileName(const std::string& filepath);
};
