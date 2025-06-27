#pragma once
#include "../Core/BlockData.h"
#include "IGeneratorStrategy.h"
#include "../UI/BlockUI.h"
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <algorithm>
#include <map>
#include <set>
#include <functional>
#include <tuple>

using namespace BlockUtilities;

// Forward declarations
class BlockController;
struct Vertex;
class Model;
class BlockMesh;
class Texture;
struct Transform;

struct GridPosition {
    int x, y, z;
};

struct GridCell {
    std::vector<int> possibleBlockTypes;
    bool collapsed;
    std::vector<int> blockTypeIds;
    std::vector<glm::vec3> blockPositions;
    std::vector<int> blockRotations;
    float cellFillAmount;
    std::vector<std::pair<int, int>> possibleBlockRotationPairs;
};

class BlockGenerator : public IGeneratorStrategy {
private:
    BlockController* controller;
    BlockUtilities::BlockData parameters;
    std::vector<std::vector<std::vector<GridCell>>> grid; // 3D grid: [x][y][z]
    Mesh* generatorMesh;

    // Animation support
    bool animationEnabled = false;
    float animationDelay = 50.0f;
    std::vector<std::tuple<int, int, int>> animationQueue; // 3D coordinates
    
    // Rotation support
    bool randomRotationsEnabled = false;
    std::vector<AssetInfo> loadedAssets;

public:
    BlockGenerator();
    BlockGenerator(BlockController* controller);
    ~BlockGenerator();
    
    // IGeneratorStrategy interface
    void Generate() override;
    Mesh* GetMesh() const { return generatorMesh; }
    
    // Parameter management
    BlockUtilities::BlockData& GetParameters() { return parameters; }
    void SetParameters(const BlockUtilities::BlockData& params) { parameters = params; }
    
    // Asset detection
    void DetectCellSizeFromAssets();
    
    // Animation control
    void SetAnimationEnabled(bool enabled) { animationEnabled = enabled; }
    void SetAnimationDelay(float delay) { animationDelay = delay; }
    bool IsAnimationInProgress() const { return !animationQueue.empty(); }
    void UpdateAnimation();

    // Rotation methods
    void ApplyRandomRotationsToGrid();
    void SetRandomRotationsEnabled(bool enabled) { randomRotationsEnabled = enabled; }
    bool IsRandomRotationsEnabled() const { return randomRotationsEnabled; }
    
    // Block statistics and management
    std::map<int, int> GetCurrentBlockCounts() const { return parameters.generationSettings.currentBlockCounts; }

private:
    // Initialization
    void initializeDefaults();
    void initializeGrid();
    void initializeSocketSystem();  // NEW: Socket system initialization
    
    // Core WFC methods
    std::vector<int> getAllBlockTypes();
    bool placeRandomBlockAt(int x, int y, int z);
    GridPosition findLowestEntropyCell();
    bool collapseCell(int x, int y, int z);
    void updateCellPossibilities(int x, int y, int z);
    
    // Generation methods
    void generateGridMultithreaded();
    void generateGridWithAnimation();
    void processRemainingCells();
    
    // NEW: Socket-based constraint validation
    bool isBlockValidAtPosition(int x, int y, int z, int blockId, int rotation);
    bool validateNeighborCompatibility(int blockId, int rotation, int faceIndex, const GridCell& neighborCell, int neighborX, int neighborY, int neighborZ);
    bool canBlocksConnectWithSockets(int blockId1, int rotation1, int face1, int blockId2, int rotation2, int face2);
    void propagateConstraints(int x, int y, int z);
    
    // NEW: Face/rotation utilities
    int getFaceIndex(const std::string& faceDirection);
    std::string getFaceDirection(int faceIndex);
    int getOppositeFaceIndex(int faceIndex);
    int getRandomRotationIndex() const;
    
    // Utility methods
    bool isValidGridPosition(int x, int y, int z) const;
    glm::vec3 calculateBlockPosition(int x, int y, int z) const;

    // World management
    void updateWorldDimensions();
    glm::vec3 calculateModelBounds(const std::shared_ptr<Model>& model);
    
    // Mesh generation (handles rotations)
    BlockMesh* generateMeshFromGrid();
    BlockMesh* createEmptyMesh();
    void addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position, int rotation,
                       std::set<std::shared_ptr<Texture>>& uniqueTextures);
    void collectTexturesFromModel(const std::shared_ptr<Model>& model, 
                                 std::set<std::shared_ptr<Texture>>& uniqueTextures);

    // Helper method for random rotations
    float getRandomYRotation() const;

    // Block count and weight management
    void initializeBlockWeights();
    void resetBlockCounts();
    bool canPlaceBlock(int blockId) const;
    int selectWeightedBlock(const std::vector<int>& validBlocks);
    void incrementBlockCount(int blockId);

    // Block selection and management methods
    int selectByWeight(const std::vector<int>& blocks);
    bool isUnlimitedBlock(int blockId) const;
    std::vector<int> getAvailableBlocks() const;
    bool hasReachedLimit(int blockId) const;
};

