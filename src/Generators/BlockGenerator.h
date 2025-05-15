#pragma once

#include "IGeneratorStrategy.h"
#include "../Core/BlockData.h"
#include "../Drawables/Mesh.h"
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>
#include <climits>
#include <cmath>
#include <iostream>
#include <glm/glm.hpp>

struct GridCell {
    std::vector<int> possibleBlockTypes;
    bool collapsed;
    int blockTypeId;
};

struct GridPosition {
    int x;
    int z;
};

class BlockGenerator : public IGeneratorStrategy {
public:
    BlockGenerator();
    ~BlockGenerator();

    void Generate() override;
    Mesh* GetMesh() const override {
        return generatorMesh;
    }

    void SetParameters(BlockUtilities::BlockData params) {
        parameters = params;
    }

    BlockUtilities::BlockData GetParameters() {
        return parameters;
    }
    
private:
    void CalculateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    glm::vec3 getColor(float height);

    struct ThreadTask {
        unsigned int startI, endI; // Range of rows (length) to process
        unsigned int startJ, endJ; // Range of columns (width) to process
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
    };

    // Parameters for generation
    BlockUtilities::BlockData parameters;
    
    // Grid of cells for Wave Function Collapse
    std::vector<std::vector<GridCell>> grid;
    Mesh* generatorMesh = nullptr;
    
    // WFC algorithm methods
    void initializeGrid();
    std::vector<int> getAllBlockTypes();
    bool placeRandomBlockAt(int x, int z);
    bool hasUnresolvedCells();
    GridPosition findLowestEntropyCell();
    bool collapseCell(int x, int z);
    void propagateConstraints(int x, int z);
    void updatePossibleBlockTypes(int x, int z, int nx, int nz);
    
    // Mesh generation methods
    Mesh* generateMeshFromGrid();
    void addBlockToMesh(int x, int z, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    Mesh* createEmptyMesh();
};
