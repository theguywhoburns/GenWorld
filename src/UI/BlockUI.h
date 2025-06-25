#pragma once

#include "../Core/BlockData.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <functional>

using namespace BlockUtilities;

// Forward declarations
class IBlockUIController;
class Model;

struct AssetInfo {
    int id;
    std::string name;
    std::string blockPath;
    std::shared_ptr<Model> model;
};

class BlockUI {
private:
    IBlockUIController* controller;
    BlockUtilities::BlockData parameters;
    std::vector<AssetInfo> loadedAssets;
    
    // Error handling
    std::string lastError;
    bool showModelError = false;
    
    // Constraint management
    std::map<int, BlockConstraints> uiConstraints;
    int selectedBlockId = -1;
    
    // Generation settings
    struct GenerationSettings {
        int threadCount = 4;
        bool useConstraints = true;
        int gridWidth = 20;
        int gridHeight = 20;
        float blockScale = 1.0f;
        bool enablePoppingAnimation = false;
        float animationDelay = 50.0f;
    } genSettings;
    
    // Helper methods
    void resetConstraintsToDefault();
    BlockConstraints createDefaultConstraints(int blockId);
    void updateConstraintConnection(BlockFaceConstraints* face, int assetId, bool isAllowed);
    std::string GetFileName(const std::string& filepath);
    
public:
    BlockUI(IBlockUIController* controller);
    ~BlockUI();
    
    // Main UI display methods
    void DisplayUI();
    void DisplayBasicSettings();
    void DisplayConstraints();
    
    // File dialog methods
    void OpenModelFileDialog();
    
    // Model loading callbacks
    void OnModelLoaded(std::shared_ptr<Model> model, const std::string& filepath);
    void OnModelLoadError(const std::string& error);
    
    // Data access methods
    BlockUtilities::BlockData& GetParameters() { return parameters; }
    const std::vector<AssetInfo>& GetLoadedAssets() const { return loadedAssets; }
    
    // Asset management
    void AddAsset(const AssetInfo& asset);
    void RemoveAsset(int id);
    
    // Constraint management
    void SetConstraints(const std::map<int, BlockConstraints>& constraints);
    const std::map<int, BlockConstraints>& GetConstraints() const;
    
    // Settings access
    const GenerationSettings& GetGenerationSettings() const { return genSettings; }
    
    // Animation settings access
    bool IsAnimationEnabled() const { return genSettings.enablePoppingAnimation; }
    float GetAnimationDelay() const { return genSettings.animationDelay; }
};
