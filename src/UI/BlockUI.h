#pragma once

#include "../Core/BlockData.h"
#include <vector>
#include <string>
#include <memory>
#include <map>

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
    
    bool showModelError = false;
    std::string lastError;

public:
    BlockUI(IBlockUIController* controller);
    ~BlockUI();
    
    void DisplayUI();
    void DisplayGridSettings();
    void DisplayBlockSettings();
    void DisplayAssetManagement();
    void DisplayDirectionalConstraints();
    
    void OpenModelFileDialog();
    
    void OnModelLoaded(std::shared_ptr<Model> model, const std::string& filepath);
    void OnModelLoadError(const std::string& error);
    
    BlockUtilities::BlockData& GetParameters() { return parameters; }
    const std::vector<AssetInfo>& GetLoadedAssets() const { return loadedAssets; }
    
private:
    std::string GetFileName(const std::string& filepath);
};
