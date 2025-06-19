#include "BlockGenerator.h"

// Include all necessary headers here
#include "../Controllers/BlockController.h"
#include "../UI/BlockUI.h"
#include "../Drawables/Model.h"
#include "../Core/Vertex.h"
#include <random>
#include <iostream>
#include <algorithm>
#include <cfloat>
#include <set>
#include "../Drawables/BlockMesh.h"


BlockGenerator::BlockGenerator() {
    this->controller = nullptr;
    
    // Initialize with block count instead of world units
    parameters.gridWidth = 20;     // 20 blocks wide
    parameters.gridLength = 20;    // 20 blocks long
    parameters.cellWidth = 5.0f;   // Each block is 5 units wide
    parameters.cellLength = 5.0f;  // Each block is 5 units long
    
    // Calculate world dimensions
    updateWorldDimensions();
    
    parameters.blockScale = 1.0f;
    generatorMesh = nullptr;
}

BlockGenerator::BlockGenerator(BlockController* controller) {
    this->controller = controller;
    
    // Initialize with block count instead of world units
    parameters.gridWidth = 20;     // 20 blocks wide
    parameters.gridLength = 20;    // 20 blocks long
    parameters.cellWidth = 5.0f;   // Each block is 5 units wide
    parameters.cellLength = 5.0f;  // Each block is 5 units long
    
    // Calculate world dimensions
    updateWorldDimensions();
    
    parameters.blockScale = 1.0f;
    generatorMesh = nullptr;
}

BlockGenerator::~BlockGenerator() {
    if (generatorMesh != nullptr) {
        delete generatorMesh;
        generatorMesh = nullptr;
    }
}

void BlockGenerator::Generate() {
    // Auto-detect cell size from loaded assets if not already detected
    if (!parameters.dimensionsDetected) {
        DetectCellSizeFromAssets();
    }
    
    // 1. Initialize the grid with all possible states
    initializeGrid();

    // 2. Start with a seed block in the center of the grid
    int centerX = parameters.gridWidth / 2;
    int centerZ = parameters.gridLength / 2;
    
    if (!placeRandomBlockAt(centerX, centerZ)) {
        std::cerr << "Failed to place initial block at center" << std::endl;
        generatorMesh = createEmptyMesh();
        return;
    }
    
    // 3. Fill the grid using simple random placement (no constraints for now)
    while (hasUnresolvedCells()) {
        GridPosition nextPos = findLowestEntropyCell();
        
        if (nextPos.x == -1 || nextPos.z == -1) {
            break;
        }
        
        if (!collapseCell(nextPos.x, nextPos.z)) {
            std::cerr << "Failed to collapse cell at " << nextPos.x << ", " << nextPos.z << std::endl;
            continue;
        }
    }
    
    // 4. Generate the final mesh from the collapsed grid
    generatorMesh = generateMeshFromGrid();
}

// Add this new method to calculate world dimensions
void BlockGenerator::updateWorldDimensions() {
    parameters.worldWidth = parameters.gridWidth * parameters.cellWidth;
    parameters.worldLength = parameters.gridLength * parameters.cellLength;
    parameters.halfWorldWidth = parameters.worldWidth / 2.0f;
    parameters.halfWorldLength = parameters.worldLength / 2.0f;
}

// Update DetectCellSizeFromAssets method
void BlockGenerator::DetectCellSizeFromAssets() {
    if (!controller || !controller->GetBlockUI()) {
        return;
    }
    
    auto assets = controller->GetBlockUI()->GetLoadedAssets();
    if (assets.empty()) {
        return;
    }
    
    // Use the first asset to determine cell size
    const auto& firstAsset = assets[0];
    if (firstAsset.model) {
        glm::vec3 bounds = calculateModelBounds(firstAsset.model);
        
        parameters.detectedBlockWidth = bounds.x;
        parameters.detectedBlockLength = bounds.z;
        parameters.detectedBlockHeight = bounds.y;
        parameters.dimensionsDetected = true;
        
        // Set cell dimensions to the actual block dimensions
        parameters.cellWidth = bounds.x;
        parameters.cellLength = bounds.z;
        
        // Recalculate world dimensions based on new cell size
        updateWorldDimensions();
    }
}

glm::vec3 BlockGenerator::calculateModelBounds(const std::shared_ptr<Model>& model) {
    if (!model || model->getMeshes().empty()) {
        return glm::vec3(5.0f); // Default size if no model
    }
    
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);
    
    // Calculate bounds from all meshes in the model
    for (auto* mesh : model->getMeshes()) {
        if (!mesh) continue;
        
        for (const auto& vertex : mesh->vertices) {
            minBounds = glm::min(minBounds, vertex.Position);
            maxBounds = glm::max(maxBounds, vertex.Position);
        }
    }
    
    return maxBounds - minBounds;
}

void BlockGenerator::initializeGrid() {
    grid.clear();
    grid.resize(parameters.gridWidth);  // Use gridWidth instead of numCellsWidth
    
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        grid[x].resize(parameters.gridLength);  // Use gridLength instead of numCellsLength
        
        for (unsigned int z = 0; z < parameters.gridLength; z++) {
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
    if (x < 0 || x >= (int)parameters.gridWidth || 
        z < 0 || z >= (int)parameters.gridLength) {
        return false;
    }
    
    auto& cell = grid[x][z];
    if (cell.possibleBlockTypes.empty()) {
        return false;
    }
    
    int randomIndex = rand() % cell.possibleBlockTypes.size();
    int chosenBlockType = cell.possibleBlockTypes[randomIndex];
    
    // Calculate position using new grid system
    float cellCenterX = (x * parameters.cellWidth) - parameters.halfWorldWidth;
    float cellCenterZ = (z * parameters.cellLength) - parameters.halfWorldLength;
    glm::vec3 blockPosition = glm::vec3(cellCenterX, 0, cellCenterZ);
    
    cell.collapsed = true;
    cell.possibleBlockTypes.clear();
    cell.possibleBlockTypes.push_back(chosenBlockType);
    
    cell.blockTypeIds.clear();
    cell.blockTypeIds.push_back(chosenBlockType);
    cell.blockPositions.clear();
    cell.blockPositions.push_back(blockPosition);
    
    return true;
}

bool BlockGenerator::hasUnresolvedCells() {
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int z = 0; z < parameters.gridLength; z++) {
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
    
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int z = 0; z < parameters.gridLength; z++) {
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
    if (x < 0 || x >= (int)parameters.gridWidth || 
        z < 0 || z >= (int)parameters.gridLength) {
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
    float cellCenterX = (x * parameters.cellWidth) - parameters.halfWorldWidth;
    float cellCenterZ = (z * parameters.cellLength) - parameters.halfWorldLength;
    glm::vec3 blockPosition = glm::vec3(cellCenterX, 0, cellCenterZ);
    
    // Add the block
    cell.blockTypeIds.push_back(chosenBlockType);
    cell.blockPositions.push_back(blockPosition);
    
    return true;
}

glm::vec3 BlockGenerator::calculateBlockPositionInCell(int x, int z, int blockIndex, const std::vector<glm::vec3>& existingPositions) {
    // Simply return the center of the cell
    float cellCenterX = (x * parameters.cellWidth) - parameters.halfWorldWidth;
    float cellCenterZ = (z * parameters.cellLength) - parameters.halfWorldLength;
    
    return glm::vec3(cellCenterX, 0, cellCenterZ);
}

bool BlockGenerator::isValidBlockPosition(int x, int z, const glm::vec3& position, int blockType) {
    // No distance constraints, always valid
    return true;
}

BlockMesh* BlockGenerator::generateMeshFromGrid() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;
    
    std::set<std::shared_ptr<Texture>> uniqueTextures;
    
    // Create the BlockMesh first
    BlockMesh* blockMesh = new BlockMesh(vertices, indices, parameters, textures);
    
    // Add block instances instead of combining vertices
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int z = 0; z < parameters.gridLength; z++) {
            const auto& cell = grid[x][z];
            if (cell.collapsed) {
                for (size_t i = 0; i < cell.blockTypeIds.size(); i++) {
                    int blockId = cell.blockTypeIds[i];
                    glm::vec3 position = cell.blockPositions[i];
                    
                    // Create transform for this block
                    Transform blockTransform;
                    blockTransform.setPosition(position);
                    blockTransform.setScale(parameters.blockScale);
                    
                    // Find the asset path for this block ID
                    if (controller && controller->GetBlockUI()) {
                        auto assets = controller->GetBlockUI()->GetLoadedAssets();
                        
                        for (const auto& asset : assets) {
                            if (asset.id == blockId) {
                                // Add block instance to BlockMesh
                                blockMesh->AddBlockInstance(asset.blockPath, blockTransform);
                                
                                // Collect textures
                                if (asset.model) {
                                    for (auto* mesh : asset.model->getMeshes()) {
                                        if (mesh) {
                                            for (auto& texture : mesh->textures) {
                                                uniqueTextures.insert(texture);
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    } else {
                        // Fallback: add by ID for simple cubes
                        blockMesh->AddBlockInstance(blockId, blockTransform);
                    }
                }
            }
        }
    }
    
    // Set collected textures
    std::vector<std::shared_ptr<Texture>> textureVector(uniqueTextures.begin(), uniqueTextures.end());
    blockMesh->SetBlockTextures(textureVector);
    
    return blockMesh;
}

void BlockGenerator::addBlockToMeshAtPosition(const glm::vec3& worldPos, int blockId, 
                                            std::vector<Vertex>& vertices, 
                                            std::vector<unsigned int>& indices) {
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
        
        if (modelMesh && !modelMesh->vertices.empty()) {
            unsigned int baseIndex = vertices.size();
            
            // DON'T modify vertices - just copy them as-is
            for (const auto& v : modelMesh->vertices) {
                vertices.push_back(v); // Copy original vertex data
            }
            
            // Add indices
            for (unsigned int idx : modelMesh->indices) {
                indices.push_back(baseIndex + idx);
            }
            
            // Use Transform system for positioning/scaling later
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

BlockMesh* BlockGenerator::createEmptyMesh() {
    return new BlockMesh(std::vector<Vertex>(), std::vector<unsigned int>(), parameters);
}


void positionBlock(BlockMesh* blockMesh, const glm::vec3& worldPos, float scale) {
    blockMesh->setPosition(worldPos);
    blockMesh->setScale(scale);
}


