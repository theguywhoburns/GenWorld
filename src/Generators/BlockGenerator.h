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
    
    bool placeRandomBlockAt(int x, int y, int z, std::mt19937& rng);
    double calculateCellEntropy(const GridCell& cell) const;
    //GridPosition findLowestEntropyCell(std::mt19937& rng);
    bool collapseCell(int x, int y, int z, std::mt19937& rng);
    void updateCellPossibilities(int x, int y, int z);

    void buildAdjacencyTable();
    void generateGridFrontierWFC(std::mt19937& rng);
    //void generateGridMultithreaded(std::mt19937& mainRng);
    void processRemainingCells(std::mt19937& rng);

    // Socket-based constraint validation
    bool isBlockValidAtPosition(int x, int y, int z, int blockId, int rotation);
    bool validateNeighborCompatibility(int blockId, int rotation, int faceIndex, const GridCell& neighborCell, int neighborX, int neighborY, int neighborZ);
    //bool canBlocksConnectWithSockets(int blockId1, int rotation1, int face1, int blockId2, int rotation2, int face2);
    void propagateConstraints(int x, int y, int z);

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
    void addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position, int rotation,
                        std::set<std::shared_ptr<Texture>>& uniqueTextures);
    void collectTexturesFromModel(const std::shared_ptr<Model>& model,
                                 std::set<std::shared_ptr<Texture>>& uniqueTextures);

    // Block count and weight management
    void initializeBlockWeights();
    void resetBlockCounts();
    bool canPlaceBlock(int blockId) const;
    int selectWeightedBlock(const std::vector<int>& validBlocks, std::mt19937& rng);
    void incrementBlockCount(int blockId);
    int selectByWeight(const std::vector<int>& blocks, std::mt19937& rng);
    std::vector<int> getAvailableBlocks() const;
    bool hasReachedLimit(int blockId) const;
};