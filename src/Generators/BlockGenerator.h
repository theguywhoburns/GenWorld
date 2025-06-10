#pragma once
#include "../Core/BlockData.h"
#include "IGeneratorStrategy.h"
#include <vector>
#include <glm/glm.hpp>

// Forward declarations
class BlockController;
struct Vertex;

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
    Mesh* GetMesh() const override { return generatorMesh; }
    BlockUtilities::BlockData& GetParameters() { return parameters; }
    void SetParameters(const BlockUtilities::BlockData& params) {
        parameters = params;
    }

private:
    void initializeGrid();
    std::vector<int> getAllBlockTypes();
    bool placeRandomBlockAt(int x, int z);
    bool hasUnresolvedCells();
    GridPosition findLowestEntropyCell();
    bool collapseCell(int x, int z);
    Mesh* generateMeshFromGrid();

    // Existing methods (keep these):
    
    void collapseGrid();
    GridPosition selectCellToCollapse();
    void propagateConstraints(int x, int z);
    void updateNeighborConstraints(int neighborX, int neighborZ, int sourceX, int sourceZ);
    bool areBlockTypesCompatible(int blockType1, int blockType2);
    
    Mesh* createEmptyMesh();
    Mesh* createMeshFromGrid();
    void addBlockToMeshAtPosition(const glm::vec3& worldPos, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    void addSimpleCubeAtPosition(const glm::vec3& worldPos, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    
    int calculateBlocksForCell(int x, int z);
    bool placeBlockInCell(int x, int z, int blockIndex);
    glm::vec3 calculateBlockPositionInCell(int x, int z, int blockIndex, const std::vector<glm::vec3>& existingPositions);
    bool isValidBlockPosition(int x, int z, const glm::vec3& position, int blockType);
};

