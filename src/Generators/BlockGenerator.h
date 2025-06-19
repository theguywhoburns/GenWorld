#pragma once
#include "../Core/BlockData.h"
#include "IGeneratorStrategy.h"
#include <vector>
#include <glm/glm.hpp>
#include <memory>
#include <algorithm>

// Forward declarations
class BlockController;
struct Vertex;
class Model;
class BlockMesh; // Add this forward declaration

struct GridPosition {
    int x, z;
};

struct GridCell {
    std::vector<int> possibleBlockTypes;
    bool collapsed;
    std::vector<int> blockTypeIds;
    std::vector<glm::vec3> blockPositions;
    float cellFillAmount;
};

class BlockGenerator : public IGeneratorStrategy {
private:
    BlockController* controller;
    BlockUtilities::BlockData parameters;
    std::vector<std::vector<GridCell>> grid;
    Mesh* generatorMesh;

public:
    BlockGenerator();
    BlockGenerator(BlockController* controller);
    ~BlockGenerator();
    void Generate() override;
    Mesh* GetMesh() const { return generatorMesh; }
    BlockUtilities::BlockData& GetParameters() { return parameters; }
    void SetParameters(const BlockUtilities::BlockData& params) { parameters = params; }
    
    // Method to detect and set cell size from first block
    void DetectCellSizeFromAssets();

private:
    // Core WFC methods
    void initializeGrid();
    std::vector<int> getAllBlockTypes();
    bool placeRandomBlockAt(int x, int z);
    bool hasUnresolvedCells();
    GridPosition findLowestEntropyCell();
    bool collapseCell(int x, int z);
    
    BlockUtilities::BlockSide getRelativeDirection(int fromX, int fromZ, int toX, int toZ);
    BlockUtilities::BlockSide getOppositeDirection(BlockUtilities::BlockSide side);

    void updateWorldDimensions();
    
    // Mesh generation - updated methods
    BlockMesh* generateMeshFromGrid(); // Changed return type
    BlockMesh* createEmptyMesh(); // Changed return type
    void addBlockToMeshAtPosition(const glm::vec3& worldPos, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    void addSimpleCubeAtPosition(const glm::vec3& worldPos, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    
    // Block placement helpers
    int calculateBlocksForCell(int x, int z);
    bool placeBlockInCell(int x, int z, int blockIndex);
    glm::vec3 calculateBlockPositionInCell(int x, int z, int blockIndex, const std::vector<glm::vec3>& existingPositions);
    bool isValidBlockPosition(int x, int z, const glm::vec3& position, int blockType);
    
    // Model dimension detection
    glm::vec3 calculateModelBounds(const std::shared_ptr<Model>& model);
};

