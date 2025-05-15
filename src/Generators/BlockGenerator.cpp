#include "BlockGenerator.h"


BlockGenerator::BlockGenerator() {
    parameters.width = 100.0f;
    parameters.length = 100.0f;
    parameters.cellSize = 1;
    parameters.numCellsWidth = static_cast<unsigned int>(parameters.width / parameters.cellSize);
    parameters.numCellsLength = static_cast<unsigned int>(parameters.length / parameters.cellSize);
    parameters.halfWidth = parameters.width / 2.0f;
    parameters.halfLength = parameters.length / 2.0f;
    parameters.stepX = parameters.width / (parameters.numCellsWidth - 1);
    parameters.stepZ = parameters.length / (parameters.numCellsLength - 1);
}


BlockGenerator::~BlockGenerator() {
    if (generatorMesh != nullptr) {
        delete generatorMesh;
        generatorMesh = nullptr;
    }
}


void BlockGenerator::Generate() {
    // Clean up any previous mesh
    if (generatorMesh != nullptr) {
        delete generatorMesh;
        generatorMesh = nullptr;
    }

    // 1. Initialize the grid with all possible states
    initializeGrid();

    // 2. Start with a seed block in the center of the grid
    int centerX = parameters.numCellsWidth / 2;
    int centerZ = parameters.numCellsLength / 2;
    
    if (!placeRandomBlockAt(centerX, centerZ)) {
        std::cerr << "Failed to place initial block at center" << std::endl;
        generatorMesh = createEmptyMesh();
        return;
    }
    
    // 3. Propagate constraints and collapse cells until the grid is filled
    while (hasUnresolvedCells()) {
        // Find cell with lowest entropy (fewest possible states)
        GridPosition nextPos = findLowestEntropyCell();
        
        // If we couldn't find a valid cell, we're either done or have a contradiction
        if (nextPos.x == -1 || nextPos.z == -1) {
            break;
        }
        
        // Collapse this cell to a single state
        if (!collapseCell(nextPos.x, nextPos.z)) {
            std::cerr << "Failed to collapse cell at " << nextPos.x << ", " << nextPos.z << std::endl;
            // Handle failure - could try to backtrack or just continue
            continue;
        }
        
        // Propagate constraints to neighboring cells
        propagateConstraints(nextPos.x, nextPos.z);
    }
    
    // 4. Generate the final mesh from the collapsed grid
    generatorMesh = generateMeshFromGrid();
}

void BlockGenerator::initializeGrid() {
    // Reset or initialize the grid
    grid.clear();
    grid.resize(parameters.numCellsWidth);
    
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        grid[x].resize(parameters.numCellsLength);
        
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            // Initialize each cell with all possible blocks
            grid[x][z].possibleBlockTypes = getAllBlockTypes();
            grid[x][z].collapsed = false;
            grid[x][z].blockTypeId = -1;  // No specific type yet
        }
    }
}

std::vector<int> BlockGenerator::getAllBlockTypes() {
    // In the future, this would return all your block types from UI or loaded assets
    // For now, we'll return some placeholder IDs
    // Replace this when you have actual block data
    std::vector<int> blockTypes;
    
    // Placeholder: assume 10 different block types (IDs 0-9)
    for (int i = 0; i < 10; i++) {
        blockTypes.push_back(i);
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
    
    // Choose a random possible block for this cell
    int randomIndex = rand() % cell.possibleBlockTypes.size();
    int chosenBlockType = cell.possibleBlockTypes[randomIndex];
    
    // Collapse the cell to this block type
    cell.collapsed = true;
    cell.possibleBlockTypes.clear();
    cell.possibleBlockTypes.push_back(chosenBlockType);
    cell.blockTypeId = chosenBlockType;
    
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
    
    // Add some randomness to avoid patterns
    std::vector<GridPosition> candidates;
    
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            auto& cell = grid[x][z];
            
            // Skip already collapsed cells or cells with no options
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
    
    // If we found candidates, choose one at random
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
    
    // Choose a random possible block for this cell
    // In a full implementation, you'd weight this by frequency
    int randomIndex = rand() % cell.possibleBlockTypes.size();
    int chosenBlockType = cell.possibleBlockTypes[randomIndex];
    
    // Collapse the cell to this block type
    cell.collapsed = true;
    cell.possibleBlockTypes.clear();
    cell.possibleBlockTypes.push_back(chosenBlockType);
    cell.blockTypeId = chosenBlockType;
    
    return true;
}

void BlockGenerator::propagateConstraints(int x, int z) {
    // Queue of cells to update
    std::queue<GridPosition> queue;
    queue.push({x, z});
    
    while (!queue.empty()) {
        GridPosition current = queue.front();
        queue.pop();
        
        // Get the current cell's constraints
        auto& currentCell = grid[current.x][current.z];
        
        // Check all four neighbors
        const int dx[] = {1, 0, -1, 0}; // right, down, left, up
        const int dz[] = {0, 1, 0, -1};
        
        for (int dir = 0; dir < 4; dir++) {
            int nx = current.x + dx[dir];
            int nz = current.z + dz[dir];
            
            // Skip neighbors outside the grid
            if (nx < 0 || nx >= (int)parameters.numCellsWidth || 
                nz < 0 || nz >= (int)parameters.numCellsLength) {
                continue;
            }
            
            auto& neighborCell = grid[nx][nz];
            
            // Skip already collapsed neighbors
            if (neighborCell.collapsed) {
                continue;
            }
            
            // Get the original size for comparison
            int originalSize = neighborCell.possibleBlockTypes.size();
            
            // Update the neighbor's possible blocks based on current cell's constraints
            updatePossibleBlockTypes(current.x, current.z, nx, nz);
            
            // If the neighbor's options changed, add it to the queue
            if (originalSize != neighborCell.possibleBlockTypes.size()) {
                queue.push({nx, nz});
            }
        }
    }
}

void BlockGenerator::updatePossibleBlockTypes(int x, int z, int nx, int nz) {
    // This would use your actual constraints between block types
    // For now, just a placeholder implementation
    auto& cell = grid[x][z];
    auto& neighborCell = grid[nx][nz];
    
    // In a real implementation, you would check which blocks can be 
    // adjacent to the blocks in the current cell based on their constraints
    // For this placeholder, we'll just randomly remove some options
    
    if (!neighborCell.possibleBlockTypes.empty() && cell.blockTypeId != -1) {
        // Placeholder logic: blocks can only be adjacent to blocks with IDs ±2 of their own ID
        std::vector<int> validOptions;
        
        for (int blockType : neighborCell.possibleBlockTypes) {
            if (std::abs(blockType - cell.blockTypeId) <= 2) {
                validOptions.push_back(blockType);
            }
        }
        
        if (!validOptions.empty()) {
            neighborCell.possibleBlockTypes = validOptions;
        }
    }
}

Mesh* BlockGenerator::generateMeshFromGrid() {
    // This will generate a mesh based on the collapsed grid
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;
    
    // Create vertices and indices based on the grid
    for (unsigned int x = 0; x < parameters.numCellsWidth; x++) {
        for (unsigned int z = 0; z < parameters.numCellsLength; z++) {
            if (grid[x][z].collapsed && grid[x][z].blockTypeId != -1) {
                // Add the block's geometry to the mesh
                addBlockToMesh(x, z, grid[x][z].blockTypeId, vertices, indices);
            }
        }
    }
    
    // Create and return the mesh
    return new Mesh(vertices, indices, textures);
}

void BlockGenerator::addBlockToMesh(int x, int z, int blockId, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    // Calculate position in world coordinates
    float worldX = (x - parameters.halfWidth) * parameters.cellSize;
    float worldZ = (z - parameters.halfLength) * parameters.cellSize;
    
    // In a real implementation, you'd get the block's geometry based on its ID
    // For this placeholder, we'll just create a simple cube for each block
    
    float blockSize = parameters.cellSize * 0.8f; // Slightly smaller than the cell for visibility
    float halfSize = blockSize / 2.0f;
    float blockHeight = blockId * 0.5f + 1.0f; // Height varies based on block ID
    
    // Define the 8 corners of the cube
    glm::vec3 corners[8] = {
        glm::vec3(worldX - halfSize, 0, worldZ - halfSize),               // Bottom left back
        glm::vec3(worldX + halfSize, 0, worldZ - halfSize),               // Bottom right back
        glm::vec3(worldX + halfSize, 0, worldZ + halfSize),               // Bottom right front
        glm::vec3(worldX - halfSize, 0, worldZ + halfSize),               // Bottom left front
        glm::vec3(worldX - halfSize, blockHeight, worldZ - halfSize),     // Top left back
        glm::vec3(worldX + halfSize, blockHeight, worldZ - halfSize),     // Top right back
        glm::vec3(worldX + halfSize, blockHeight, worldZ + halfSize),     // Top right front
        glm::vec3(worldX - halfSize, blockHeight, worldZ + halfSize)      // Top left front
    };
    
    // Define the 6 faces of the cube (2 triangles per face)
    unsigned int baseIndex = vertices.size();
    unsigned int faceIndices[6][6] = {
        {0, 1, 2, 0, 2, 3},     // Bottom
        {4, 7, 6, 4, 6, 5},     // Top
        {0, 4, 5, 0, 5, 1},     // Back
        {1, 5, 6, 1, 6, 2},     // Right
        {2, 6, 7, 2, 7, 3},     // Front
        {3, 7, 4, 3, 4, 0}      // Left
    };
    
    // Create vertices for each corner
    for (int i = 0; i < 8; i++) {
        Vertex vertex;
        vertex.Position = corners[i];
        
        // Set color based on block ID
        float r = (blockId % 3) / 2.0f + 0.25f;
        float g = ((blockId / 3) % 3) / 2.0f + 0.25f;
        float b = ((blockId / 9) % 3) / 2.0f + 0.25f;
        vertex.Color = glm::vec3(r, g, b);
        
        // Set normal (simplified)
        vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // Set texture coordinates (simplified)
        vertex.TexCoords = glm::vec2((i % 4) / 3.0f, (i / 4) / 3.0f);
        
        vertices.push_back(vertex);
    }
    
    // Add indices for the 6 faces
    for (int face = 0; face < 6; face++) {
        for (int idx = 0; idx < 6; idx++) {
            indices.push_back(baseIndex + faceIndices[face][idx]);
        }
    }
}

Mesh* BlockGenerator::createEmptyMesh() {
    return new Mesh(std::vector<Vertex>(), std::vector<unsigned int>(), std::vector<std::shared_ptr<Texture>>());
}
