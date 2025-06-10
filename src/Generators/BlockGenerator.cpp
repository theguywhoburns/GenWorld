#include "BlockGenerator.h"

// Include all necessary headers here
#include "../Controllers/BlockController.h"
#include "../UI/BlockUI.h"
#include "../Drawables/Model.h"
#include "../Core/Vertex.h"
#include <random>


BlockGenerator::BlockGenerator() {
    this->controller = nullptr;
    parameters.width = 100.0f;
    parameters.length = 100.0f;
    parameters.cellSize = 5;
    parameters.numCellsWidth = static_cast<unsigned int>(parameters.width / parameters.cellSize);
    parameters.numCellsLength = static_cast<unsigned int>(parameters.length / parameters.cellSize);
    parameters.halfWidth = parameters.width / 2.0f;
    parameters.halfLength = parameters.length / 2.0f;
    parameters.blockScale = 1.0f;  // Initialize block scale
    generatorMesh = nullptr;
}

BlockGenerator::BlockGenerator(BlockController* controller) {
    this->controller = controller;
    parameters.width = 100.0f;
    parameters.length = 100.0f;
    parameters.cellSize = 5;
    parameters.numCellsWidth = static_cast<unsigned int>(parameters.width / parameters.cellSize);
    parameters.numCellsLength = static_cast<unsigned int>(parameters.length / parameters.cellSize);
    parameters.halfWidth = parameters.width / 2.0f;
    parameters.halfLength = parameters.length / 2.0f;
    parameters.blockScale = 1.0f;  // Initialize block scale
    generatorMesh = nullptr;
}

BlockGenerator::~BlockGenerator() {
    if (generatorMesh != nullptr) {
        delete generatorMesh;
        generatorMesh = nullptr;
    }
}

void BlockGenerator::Generate() {
    // 1. Initialize the grid with all possible states
    initializeGrid();

    // 2. Start with a seed block in the center of the grid
    int centerX = parameters.numCellsWidth / 2;
    int centerZ = parameters.numCellsLength / 2;
    
    // DEBUG: Print center coordinates
    std::cout << "Placing center block at grid position: " << centerX << ", " << centerZ << std::endl;
    std::cout << "Grid dimensions: " << parameters.numCellsWidth << " x " << parameters.numCellsLength << std::endl;
    
    if (!placeRandomBlockAt(centerX, centerZ)) {
        std::cerr << "Failed to place initial block at center" << std::endl;
        generatorMesh = createEmptyMesh();
        return;
    }
    
    // 3. Propagate constraints and collapse cells until the grid is filled
    while (hasUnresolvedCells()) {
        GridPosition nextPos = findLowestEntropyCell();
        
        if (nextPos.x == -1 || nextPos.z == -1) {
            break;
        }
        
        if (!collapseCell(nextPos.x, nextPos.z)) {
            std::cerr << "Failed to collapse cell at " << nextPos.x << ", " << nextPos.z << std::endl;
            continue;
        }
        
        propagateConstraints(nextPos.x, nextPos.z);
    }
    
    // 4. Generate the final mesh from the collapsed grid
    generatorMesh = generateMeshFromGrid();
}

void BlockGenerator::initializeGrid() {
    grid.clear();
    grid.resize(parameters.numCellsWidth);
    
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        grid[x].resize(parameters.numCellsLength);
        
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            grid[x][z].possibleBlockTypes = getAllBlockTypes();
            grid[x][z].collapsed = false;
            grid[x][z].blockTypeIds.clear();
            grid[x][z].blockPositions.clear();
            grid[x][z].cellFillAmount = 0.0f;
        }
    }
}

std::vector<int> BlockGenerator::getAllBlockTypes() {
    std::vector<int> blockTypes;
    
    // Get asset IDs from the UI via controller
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        
        if (assets.empty()) {
            // Fallback to placeholder blocks if no assets are loaded
            for (int i = 0; i < 10; i++) {
                blockTypes.push_back(i);
            }
        } else {
            // Use loaded asset IDs
            for (const auto& asset : assets) {
                blockTypes.push_back(asset.id);
            }
        }
    } else {
        // Fallback if no controller
        for (int i = 0; i < 10; i++) {
            blockTypes.push_back(i);
        }
    }
    
    return blockTypes;
}

bool BlockGenerator::placeRandomBlockAt(int x, int z) {
    if (x < 0 || x >= (int)parameters.numCellsWidth || 
        z < 0 || z >= (int)parameters.numCellsLength) {
        return false;
    }
    
    auto& cell = grid[x][z];
    if (cell.possibleBlockTypes.empty()) {
        return false;
    }
    
    int randomIndex = rand() % cell.possibleBlockTypes.size();
    int chosenBlockType = cell.possibleBlockTypes[randomIndex];
    
    // FIXED: Use the same coordinate calculation as calculateBlockPositionInCell
    float cellCenterX = (x * parameters.cellSize) - parameters.halfWidth;
    float cellCenterZ = (z * parameters.cellSize) - parameters.halfLength;
    glm::vec3 blockPosition = glm::vec3(cellCenterX, 0, cellCenterZ);
    
    // DEBUG: Print center block world position
    std::cout << "Center block world position: " << blockPosition.x << ", " << blockPosition.y << ", " << blockPosition.z << std::endl;
    
    cell.collapsed = true;
    cell.possibleBlockTypes.clear();
    cell.possibleBlockTypes.push_back(chosenBlockType);
    
    // Use the new vector-based structure
    cell.blockTypeIds.clear();
    cell.blockTypeIds.push_back(chosenBlockType);
    cell.blockPositions.clear();
    cell.blockPositions.push_back(blockPosition);
    
    return true;
}

bool BlockGenerator::hasUnresolvedCells() {
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            if (!grid[x][z].collapsed && !grid[x][z].possibleBlockTypes.empty()) {
                return true;
            }
        }
    }
    return false;
}

GridPosition BlockGenerator::findLowestEntropyCell() {
    GridPosition lowestPos = {-1, -1};
    int lowestEntropy = INT_MAX;
    std::vector<GridPosition> candidates;
    
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            auto& cell = grid[x][z];
            
            if (cell.collapsed || cell.possibleBlockTypes.empty()) {
                continue;
            }
            
            int entropy = cell.possibleBlockTypes.size();
            
            if (entropy < lowestEntropy) {
                lowestEntropy = entropy;
                candidates.clear();
                candidates.push_back({(int)x, (int)z});
            } 
            else if (entropy == lowestEntropy) {
                candidates.push_back({(int)x, (int)z});
            }
        }
    }
    
    if (!candidates.empty()) {
        int randomIndex = rand() % candidates.size();
        return candidates[randomIndex];
    }
    
    return lowestPos;
}

bool BlockGenerator::collapseCell(int x, int z) {
    if (x < 0 || x >= (int)parameters.numCellsWidth || 
        z < 0 || z >= (int)parameters.numCellsLength) {
        return false;
    }
    
    auto& cell = grid[x][z];
    if (cell.collapsed || cell.possibleBlockTypes.empty()) {
        return false;
    }
    
    // Determine how many blocks to place in this cell
    int numBlocksToPlace = calculateBlocksForCell(x, z);
    
    // Place blocks with distance constraints
    for (int i = 0; i < numBlocksToPlace; i++) {
        if (!placeBlockInCell(x, z, i)) {
            break; // Stop if we can't place more blocks
        }
    }
    
    cell.collapsed = true;
    return true;
}

int BlockGenerator::calculateBlocksForCell(int x, int z) {
    return 1;
}

bool BlockGenerator::placeBlockInCell(int x, int z, int blockIndex) {
    auto& cell = grid[x][z];
    
    if (cell.possibleBlockTypes.empty()) {
        return false;
    }
    
    // Choose block type
    int randomIndex = rand() % cell.possibleBlockTypes.size();
    int chosenBlockType = cell.possibleBlockTypes[randomIndex];
    
    // Calculate center position of cell
    float cellCenterX = (x * parameters.cellSize) - parameters.halfWidth;
    float cellCenterZ = (z * parameters.cellSize) - parameters.halfLength;
    glm::vec3 blockPosition = glm::vec3(cellCenterX, 0, cellCenterZ);
    
    // Add the block
    cell.blockTypeIds.push_back(chosenBlockType);
    cell.blockPositions.push_back(blockPosition);
    
    return true;
}

glm::vec3 BlockGenerator::calculateBlockPositionInCell(int x, int z, int blockIndex, const std::vector<glm::vec3>& existingPositions) {
    // Simply return the center of the cell
    float cellCenterX = (x * parameters.cellSize) - parameters.halfWidth;
    float cellCenterZ = (z * parameters.cellSize) - parameters.halfLength;
    
    return glm::vec3(cellCenterX, 0, cellCenterZ);
}

bool BlockGenerator::isValidBlockPosition(int x, int z, const glm::vec3& position, int blockType) {
    // No distance constraints, always valid
    return true;
}

Mesh* BlockGenerator::generateMeshFromGrid() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;
    
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            const auto& cell = grid[x][z];
            if (cell.collapsed) {
                // Add all blocks in this cell
                for (size_t i = 0; i < cell.blockTypeIds.size(); i++) {
                    addBlockToMeshAtPosition(
                        cell.blockPositions[i], 
                        cell.blockTypeIds[i], 
                        vertices, 
                        indices
                    );
                }
            }
        }
    }
    
    return new Mesh(vertices, indices, textures);
}

void BlockGenerator::addBlockToMeshAtPosition(const glm::vec3& worldPos, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    // Check if we have a controller and can get assets
    if (!controller || !controller->GetBlockUI()) {
        addSimpleCubeAtPosition(worldPos, blockId, vertices, indices);
        return;
    }
    
    const AssetInfo* asset = nullptr;
    auto assets = controller->GetBlockUI()->GetLoadedAssets();
    
    for (const auto& a : assets) {
        if (a.id == blockId) {
            asset = &a;
            break;
        }
    }
    
    // If we found a valid asset with a model, use it
    if (asset && asset->model && !asset->model->getMeshes().empty()) {
        Mesh* modelMesh = asset->model->getMeshes()[0];

        if (modelMesh && !modelMesh->vertices.empty() && !modelMesh->indices.empty()) {
            unsigned int baseIndex = vertices.size();
            
            // Use the blockScale parameter for model scaling
            float modelScale = parameters.blockScale;
            
            // Add all vertices from the model
            for (const auto& v : modelMesh->vertices) {
                Vertex newVertex = v;
                newVertex.Position = newVertex.Position * modelScale;
                newVertex.Position += worldPos;
                vertices.push_back(newVertex);
            }
            
            // Add all indices from the model, offset by our base index
            for (unsigned int idx : modelMesh->indices) {
                indices.push_back(baseIndex + idx);
            }
            
            return;
        }
    }
    
    // Fallback: create a simple cube
    addSimpleCubeAtPosition(worldPos, blockId, vertices, indices);
}

void BlockGenerator::addSimpleCubeAtPosition(const glm::vec3& worldPos, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    // Use the blockScale parameter for cube size
    float blockSize = parameters.blockScale;
    float halfSize = blockSize / 2.0f;
    float blockHeight = blockId * 0.5f + 1.0f;
    
    // Use the exact world position passed in
    float worldX = worldPos.x;
    float worldZ = worldPos.z;
    
    // Define the 8 corners of the cube
    glm::vec3 corners[8] = {
        glm::vec3(worldX - halfSize, 0, worldZ - halfSize),
        glm::vec3(worldX + halfSize, 0, worldZ - halfSize),
        glm::vec3(worldX + halfSize, 0, worldZ + halfSize),
        glm::vec3(worldX - halfSize, 0, worldZ + halfSize),
        glm::vec3(worldX - halfSize, blockHeight, worldZ - halfSize),
        glm::vec3(worldX + halfSize, blockHeight, worldZ - halfSize),
        glm::vec3(worldX + halfSize, blockHeight, worldZ + halfSize),
        glm::vec3(worldX - halfSize, blockHeight, worldZ + halfSize)
    };
    
    unsigned int baseIndex = vertices.size();
    
    // Lambda function to create a properly initialized vertex
    auto createVertex = [](const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& texCoord) -> Vertex {
        Vertex vertex;
        vertex.Position = pos;
        vertex.Normal = normal;
        vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
        vertex.TexCoords = texCoord;
        vertex.Tangent = glm::vec3(0.0f, 0.0f, 0.0f);
        vertex.Bitangent = glm::vec3(0.0f, 0.0f, 0.0f);
        
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
        
        return vertex;
    };
    
    // Bottom face (y = 0)
    vertices.push_back(createVertex(corners[0], glm::vec3(0, -1, 0), glm::vec2(0, 0)));
    vertices.push_back(createVertex(corners[1], glm::vec3(0, -1, 0), glm::vec2(1, 0)));
    vertices.push_back(createVertex(corners[2], glm::vec3(0, -1, 0), glm::vec2(1, 1)));
    vertices.push_back(createVertex(corners[3], glm::vec3(0, -1, 0), glm::vec2(0, 1)));
    
    // Top face (y = blockHeight)
    vertices.push_back(createVertex(corners[4], glm::vec3(0, 1, 0), glm::vec2(0, 0)));
    vertices.push_back(createVertex(corners[5], glm::vec3(0, 1, 0), glm::vec2(1, 0)));
    vertices.push_back(createVertex(corners[6], glm::vec3(0, 1, 0), glm::vec2(1, 1)));
    vertices.push_back(createVertex(corners[7], glm::vec3(0, 1, 0), glm::vec2(0, 1)));
    
    // Front face (z+)
    vertices.push_back(createVertex(corners[3], glm::vec3(0, 0, 1), glm::vec2(0, 0)));
    vertices.push_back(createVertex(corners[2], glm::vec3(0, 0, 1), glm::vec2(1, 0)));
    vertices.push_back(createVertex(corners[6], glm::vec3(0, 0, 1), glm::vec2(1, 1)));
    vertices.push_back(createVertex(corners[7], glm::vec3(0, 0, 1), glm::vec2(0, 1)));
    
    // Back face (z-)
    vertices.push_back(createVertex(corners[1], glm::vec3(0, 0, -1), glm::vec2(0, 0)));
    vertices.push_back(createVertex(corners[0], glm::vec3(0, 0, -1), glm::vec2(1, 0)));
    vertices.push_back(createVertex(corners[4], glm::vec3(0, 0, -1), glm::vec2(1, 1)));
    vertices.push_back(createVertex(corners[5], glm::vec3(0, 0, -1), glm::vec2(0, 1)));
    
    // Right face (x+)
    vertices.push_back(createVertex(corners[2], glm::vec3(1, 0, 0), glm::vec2(0, 0)));
    vertices.push_back(createVertex(corners[1], glm::vec3(1, 0, 0), glm::vec2(1, 0)));
    vertices.push_back(createVertex(corners[5], glm::vec3(1, 0, 0), glm::vec2(1, 1)));
    vertices.push_back(createVertex(corners[6], glm::vec3(1, 0, 0), glm::vec2(0, 1)));
    
    // Left face (x-)
    vertices.push_back(createVertex(corners[0], glm::vec3(-1, 0, 0), glm::vec2(0, 0)));
    vertices.push_back(createVertex(corners[3], glm::vec3(-1, 0, 0), glm::vec2(1, 0)));
    vertices.push_back(createVertex(corners[7], glm::vec3(-1, 0, 0), glm::vec2(1, 1)));
    vertices.push_back(createVertex(corners[4], glm::vec3(-1, 0, 0), glm::vec2(0, 1)));
    
    // Define indices for all faces
    for (int face = 0; face < 6; face++) {
        unsigned int faceBase = baseIndex + face * 4;
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 1);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 0);
        indices.push_back(faceBase + 2);
        indices.push_back(faceBase + 3);
    }
}

Mesh* BlockGenerator::createEmptyMesh() {
    return new Mesh(std::vector<Vertex>(), std::vector<unsigned int>(), std::vector<std::shared_ptr<Texture>>());
}

void BlockGenerator::propagateConstraints(int x, int z) {
    // After collapsing a cell, we need to update neighboring cells
    // to remove incompatible block types based on adjacency rules
    
    const int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int dz[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    
    // Check all 8 neighboring cells
    for (int i = 0; i < 8; i++) {
        int nx = x + dx[i];
        int nz = z + dz[i];
        
        // Skip if neighbor is out of bounds
        if (nx < 0 || nx >= (int)parameters.numCellsWidth || 
            nz < 0 || nz >= (int)parameters.numCellsLength) {
            continue;
        }
        
        auto& neighborCell = grid[nx][nz];
        
        // Skip if neighbor is already collapsed
        if (neighborCell.collapsed) {
            continue;
        }
        
        // Update neighbor's possible block types based on what we just placed
        updateNeighborConstraints(nx, nz, x, z);
    }
}

void BlockGenerator::updateNeighborConstraints(int neighborX, int neighborZ, int sourceX, int sourceZ) {
    auto& neighborCell = grid[neighborX][neighborZ];
    const auto& sourceCell = grid[sourceX][sourceZ];
    
    if (!sourceCell.collapsed || sourceCell.blockTypeIds.empty()) {
        return;
    }
    
    // For now, we'll implement basic constraint propagation
    // You can expand this based on your specific block rules
    
    std::vector<int> validTypes;
    
    for (int blockType : neighborCell.possibleBlockTypes) {
        bool isValid = true;
        
        // Check if this block type can be adjacent to any of the source cell's blocks
        for (int sourceBlockType : sourceCell.blockTypeIds) {
            if (!areBlockTypesCompatible(blockType, sourceBlockType)) {
                isValid = false;
                break;
            }
        }
        
        if (isValid) {
            validTypes.push_back(blockType);
        }
    }
    
    neighborCell.possibleBlockTypes = validTypes;
}

bool BlockGenerator::areBlockTypesCompatible(int blockType1, int blockType2) {
    // Check if we have constraints defined
    if (!controller || !controller->GetBlockUI()) {
        return true; // Default: all compatible if no UI
    }
    
    auto constraints = controller->GetBlockUI()->GetParameters().blockConstraints;
    
    // Find constraint for blockType1
    for (const auto& constraint : constraints) {
        if (constraint.blockTypeId == blockType1) {
            // Check if blockType2 is in the allowed neighbors
            return std::find(constraint.allowedNeighbors.begin(), 
                           constraint.allowedNeighbors.end(), 
                           blockType2) != constraint.allowedNeighbors.end();
        }
    }
    
    // If no constraint found, check the reverse (blockType2 -> blockType1)
    for (const auto& constraint : constraints) {
        if (constraint.blockTypeId == blockType2) {
            return std::find(constraint.allowedNeighbors.begin(), 
                           constraint.allowedNeighbors.end(), 
                           blockType1) != constraint.allowedNeighbors.end();
        }
    }
    
    // If no constraints defined for either block, they're compatible
    return constraints.empty();
}
