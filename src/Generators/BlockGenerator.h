#pragma once
#include "../Core/BlockData.h"
#include "IGeneratorStrategy.h"
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
    float cellFillAmount;
};

class BlockGenerator : public IGeneratorStrategy {
private:
    BlockController* controller;
    BlockUtilities::BlockData parameters;
    std::vector<std::vector<std::vector<GridCell>>> grid; // 3D grid: [x][y][z]
    Mesh* generatorMesh;
    std::map<int, BlockUtilities::BlockConstraints> blockConstraints;

    // Animation support
    bool animationEnabled = false;
    float animationDelay = 50.0f;
    std::vector<std::tuple<int, int, int>> animationQueue; // 3D coordinates
    
    // Rotation support
    bool randomRotationsEnabled = false;

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

private:
    // Initialization
    void initializeDefaults();
    void initializeGrid();
    void initializeBlockConstraints();
    
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
    
    // Constraint validation
    bool isBlockValidAtPosition(int x, int y, int z, int blockId);
    bool validateNeighborCompatibility(int blockId, const std::string& direction, const GridCell& neighborCell);
    bool canBlocksConnect(int blockId1, const std::string& face1, int blockId2, const std::string& face2);
    bool canBlocksConnectMutually(int blockId1, const std::string& face1, int blockId2, const std::string& face2);
    bool canFaceBeExposed(int blockId, const std::string& face);
    void propagateConstraints(int x, int y, int z);
    
    // Utility methods
    const BlockFaceConstraints* getFaceConstraints(const BlockConstraints& constraints, const std::string& face);
    std::string getOppositeFace(const std::string& face);
    bool isValidGridPosition(int x, int y, int z) const;
    glm::vec3 calculateBlockPosition(int x, int y, int z) const;
    
    // World management
    void updateWorldDimensions();
    glm::vec3 calculateModelBounds(const std::shared_ptr<Model>& model);
    
    // Mesh generation
    BlockMesh* generateMeshFromGrid();
    BlockMesh* createEmptyMesh();
    void addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position, 
                       std::set<std::shared_ptr<Texture>>& uniqueTextures);
    void collectTexturesFromModel(const std::shared_ptr<Model>& model, 
                                 std::set<std::shared_ptr<Texture>>& uniqueTextures);

    // Helper method for random rotations
    float getRandomYRotation() const;
};

