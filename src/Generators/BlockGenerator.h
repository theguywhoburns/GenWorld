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
#include <tuple>
#include <random>
#include <stack>
#include <queue>

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
    bool operator<(const GridPosition& other) const {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }
};

struct FrontierCell {
    double entropy;
    GridPosition pos;
    bool operator>(const FrontierCell& other) const { return entropy > other.entropy; }
};

struct GridCell {
    std::vector<std::pair<int, int>> possibleBlockRotationPairs; // (blockId, rotation)
    bool collapsed = false;
    std::vector<int> blockTypeIds;
    std::vector<int> blockRotations;
    std::vector<glm::vec3> blockPositions;
    float entropy = 0.0f;
};

class BlockGenerator : public IGeneratorStrategy {
private:
    BlockController* controller;
    BlockUtilities::BlockData parameters;
    std::vector<std::vector<std::vector<GridCell>>> grid; // 3D grid: [x][y][z]
    Mesh* generatorMesh = nullptr;

    std::map<
        std::tuple<int, int, int>, // blockId, rotation, face
        std::set<std::pair<int, int>> // neighborBlockId, neighborRotation
    > adjacencyTable;

    bool isFirstBlock = true;

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

    // Block statistics and management
    std::map<int, int> GetCurrentBlockCounts() const { return parameters.generationSettings.currentBlockCounts; }

private:
    // Initialization
    void initializeDefaults();
    void initializeGrid();
    void initializeSocketSystem();

    // Core WFC methods
    std::vector<int> getAllBlockTypes();
    void propagateWave(const GridPosition& startPos);
    std::vector<GridPosition> getNeighborPositions(const GridPosition& pos) const;
    bool validateCellPossibilitySpace(int x, int y, int z);
    bool collapseCellWFC(int x, int y, int z, std::mt19937& rng);

    bool runSingleWFCAttempt(std::mt19937& rng);
    bool isGenerationComplete() const;
    bool hasContradictions() const;
    void printGenerationSummary() const;

    double calculateCellEntropy(const GridCell& cell) const;
    void updateCellPossibilities(int x, int y, int z);

    void buildAdjacencyTable();
    void generateGridFrontierWFC(std::mt19937& rng);

    // Socket-based constraint validation
    bool isBlockValidAtPosition(int x, int y, int z, int blockId, int rotation) const;
    bool validateNeighborCompatibility(int blockId, int rotation, int faceIndex, const GridCell& neighborCell, int neighborX, int neighborY, int neighborZ) const;

    // Face/rotation utilities
    int getFaceIndex(const std::string& faceDirection);
    std::string getFaceDirection(int faceIndex);
    int getOppositeFaceIndex(int faceIndex);

    // Utility methods
    bool isValidGridPosition(int x, int y, int z) const;
    glm::vec3 calculateBlockPosition(int x, int y, int z) const;

    // World management
    void updateWorldDimensions();
    glm::vec3 calculateModelBounds(const std::shared_ptr<Model>& model);

    // Mesh generation (handles rotations)
    BlockMesh* generateMeshFromGrid();
    BlockMesh* createEmptyMesh();
    void addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position, int rotation);

    // Block count and weight management
    void initializeBlockWeights();
    void resetBlockCounts();
    void resetGridForRestart();
    bool canPlaceBlock(int blockId) const;
    int selectWeightedBlock(const std::vector<int>& validBlocks, std::mt19937& rng);
    void incrementBlockCount(int blockId);
    int selectByWeight(const std::vector<int>& blocks, std::mt19937& rng);
    std::vector<int> getAvailableBlocks() const;
    bool hasReachedLimit(int blockId) const;
    
    // Min/Max count management
    bool hasMetMinimumRequirements() const;
    std::vector<int> getBlocksNeedingMinCount() const;
    bool shouldPrioritizeMinCountBlock(int x, int y, int z, int blockId) const;

    // Rectangular castle generation
    void generateRectangularCastle(std::mt19937& rng);
    bool attemptRectangularCastleGeneration(std::mt19937& rng);
    void initializeGridMask();
    bool isGridCellMasked(int x, int y, int z) const;
    bool isCornerPosition(int x, int y, int z) const;
    GridPosition findFirstCornerPosition() const;
    bool tryPlaceCornerBlock(int x, int y, int z, std::mt19937& rng);
    int findBestCornerRotation(int x, int y, int z, int cornerBlockId) const;
    bool isCornerRotationValid(int x, int y, int z, int cornerBlockId, int rotation, bool isStartingCorner) const;
    int getCornerRotationForPosition(int x, int z) const;
    std::vector<int> getCornerBlocks() const;
    
    // Corner block preset system
    int detectCornerBlockPreset(int cornerBlockId) const;
    int getCornerPosition(int x, int z) const;
    int getRotationForPresetAtCorner(int preset, int cornerPosition) const;

private:
    std::vector<std::vector<std::vector<bool>>> gridMask; // 3D mask for rectangular castle generation
    
    // WFC propagation structures
    std::stack<GridPosition> propagationFringe;
    std::set<GridPosition> duplicateSet;
};