#pragma once

#include "GeneratorUI.h"
#include "../Core/BlockData.h"
#include <string>
#include <vector>
#include <memory>

class IBlockUIController;
class Model;

struct AssetInfo {
    int id;
    std::string name;
    std::string blockPath;
    std::shared_ptr<Model> model;
};

class BlockUI : public GeneratorUI {
public:
    BlockUI(IBlockUIController* controller);
    ~BlockUI();
    void DisplayUI() override;

    void SetParameters(BlockUtilities::BlockData params) {
        parameters = params;
    }

    BlockUtilities::BlockData GetParameters() {
        return parameters;
    }
    
    std::vector<AssetInfo> GetLoadedAssets() const {
        return loadedAssets;
    }

    // Methods for controller to call back
    void OnModelLoaded(std::shared_ptr<Model> model, const std::string& filepath);
    void OnModelLoadError(const std::string& error);

private:
    void DisplayGridSettings();
    void DisplayAssetManagement();
    void DisplayConstraints();
    void OpenModelFileDialog();
    std::string GetFileName(const std::string& path);

    BlockUtilities::BlockData parameters;
    IBlockUIController* controller;  // Use interface
    std::vector<AssetInfo> loadedAssets;
    
    // Error tracking
    bool showModelError = false;
    std::string lastError;

    // Constraint management
    void DisplayConstraintManagement();
    void AddNewConstraint();
    void RemoveConstraint(int index);
};
