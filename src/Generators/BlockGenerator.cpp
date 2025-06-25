#include "BlockGenerator.h"
#include "../Controllers/BlockController.h"
#include "../UI/BlockUI.h"
#include "../Drawables/Model.h"
#include "../Core/Vertex.h"
#include "../Drawables/BlockMesh.h"
#include <random>
#include <iostream>
#include <algorithm>
#include <cfloat>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

std::mutex gridMutex;

BlockGenerator::BlockGenerator(BlockController* controller) : controller(controller) {
    initializeDefaults();
}

BlockGenerator::BlockGenerator() : controller(nullptr) {
    initializeDefaults();
}

BlockGenerator::~BlockGenerator() {
    delete generatorMesh;
}

void BlockGenerator::initializeDefaults() {
    // Initialize with 3D parameters: {gridWidth, gridHeight, gridLength, cellWidth, cellHeight, cellLength, blockScale}
    parameters = {20, 10, 20, 5.0f, 5.0f, 5.0f, 1.0f};
    updateWorldDimensions();
    generatorMesh = nullptr;
}

void BlockGenerator::Generate() {
    initializeBlockConstraints();
    
    // Get animation and rotation settings from UI
    if (controller && controller->GetBlockUI()) {
        animationEnabled = controller->GetBlockUI()->IsAnimationEnabled();
        animationDelay = controller->GetBlockUI()->GetAnimationDelay();
        randomRotationsEnabled = controller->GetBlockUI()->IsRandomRotationEnabled();
    }
    
    if (!parameters.dimensionsDetected) {
        DetectCellSizeFromAssets();
    }
    
    initializeGrid();
    
    // Place seed block at center
    int centerX = parameters.gridWidth / 2;
    int centerY = parameters.gridHeight / 2;
    int centerZ = parameters.gridLength / 2;
    
    if (!placeRandomBlockAt(centerX, centerY, centerZ)) {
        std::cerr << "Failed to place initial block at center" << std::endl;
        generatorMesh = createEmptyMesh();
        return;
    }
    
    if (animationEnabled) {
        generateGridWithAnimation();
    } else {
        generateGridMultithreaded();
    }
    
    std::cout << "Starting mesh generation..." << std::endl;
    generatorMesh = generateMeshFromGrid();
    std::cout << "Mesh generation complete!" << std::endl;
}

void BlockGenerator::generateGridMultithreaded() {
    const unsigned int numThreads = std::min(4u, std::thread::hardware_concurrency());
    std::vector<std::thread> workerThreads;
    std::atomic<int> totalProcessed(0);
    std::atomic<bool> workCompleted(false);
    
    auto processChunk = [&](unsigned int threadId) {
        int cellsProcessed = 0;
        while (!workCompleted) {
            GridPosition nextPos{-1, -1, -1};
            bool foundWork = false;
            
            {
                std::lock_guard<std::mutex> lock(gridMutex);
                nextPos = findLowestEntropyCell();
                foundWork = (nextPos.x != -1 && nextPos.y != -1 && nextPos.z != -1);
                if (!foundWork) workCompleted = true;
            }
            
            if (!foundWork) break;
            
            {
                std::lock_guard<std::mutex> lock(gridMutex);
                if (!grid[nextPos.x][nextPos.y][nextPos.z].collapsed && 
                    collapseCell(nextPos.x, nextPos.y, nextPos.z)) {
                    cellsProcessed++;
                    totalProcessed++;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };
    
    // Launch and join worker threads
    for (unsigned int i = 0; i < numThreads; i++) {
        workerThreads.emplace_back(processChunk, i);
    }
    
    for (auto& thread : workerThreads) {
        thread.join();
    }
    
    // Process remaining cells
    processRemainingCells();
}

void BlockGenerator::generateGridWithAnimation() {
    std::cout << "Starting animated 3D grid generation..." << std::endl;
    
    // Clear animation queue
    animationQueue.clear();
    
    // Build queue of all cells that need to be processed (excluding the center seed)
    int centerX = parameters.gridWidth / 2;
    int centerY = parameters.gridHeight / 2;
    int centerZ = parameters.gridLength / 2;
    
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                // Skip the center cell (already placed)
                if ((int)x == centerX && (int)y == centerY && (int)z == centerZ) {
                    continue;
                }
                
                animationQueue.push_back({(int)x, (int)y, (int)z});
            }
        }
    }
    
    // Shuffle the queue for more interesting animation
    std::shuffle(animationQueue.begin(), animationQueue.end(), std::mt19937(std::random_device()()));
    
    std::cout << "Animation queue prepared with " << animationQueue.size() << " cells" << std::endl;
}

void BlockGenerator::UpdateAnimation() {
    if (!animationEnabled || animationQueue.empty()) {
        return;
    }
    
    static auto lastAnimationTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastAnimationTime).count();
    
    if (elapsed >= animationDelay) {
        // Process next cell in queue
        auto nextCell = animationQueue.front();
        animationQueue.erase(animationQueue.begin());
        
        int x = std::get<0>(nextCell);
        int y = std::get<1>(nextCell);
        int z = std::get<2>(nextCell);
        
        // Try to collapse this cell
        auto& cell = grid[x][y][z];
        if (!cell.collapsed) {
            updateCellPossibilities(x, y, z);
            
            if (!cell.possibleBlockTypes.empty()) {
                // Choose random valid block
                int randomIndex = rand() % cell.possibleBlockTypes.size();
                int chosenBlockType = cell.possibleBlockTypes[randomIndex];
                
                // Calculate position
                glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
                
                // Collapse the cell
                cell.collapsed = true;
                cell.possibleBlockTypes.clear();
                cell.possibleBlockTypes.push_back(chosenBlockType);
                
                cell.blockTypeIds.clear();
                cell.blockTypeIds.push_back(chosenBlockType);
                cell.blockPositions.clear();
                cell.blockPositions.push_back(blockPosition);
                
                // Propagate constraints
                propagateConstraints(x, y, z);
                
                // Regenerate mesh to show the new block
                if (generatorMesh) {
                    delete generatorMesh;
                }
                generatorMesh = generateMeshFromGrid();
                
                std::cout << "Animated block placed at (" << x << ", " << y << ", " << z << "). Remaining: " << animationQueue.size() << std::endl;
            }
        }
        
        lastAnimationTime = currentTime;
    }
    
    // Check if animation is complete
    if (animationQueue.empty()) {
        std::cout << "3D Animation complete!" << std::endl;
    }
}

void BlockGenerator::processRemainingCells() {
    int remainingCells = 0;
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                if (!grid[x][y][z].collapsed && !grid[x][y][z].possibleBlockTypes.empty()) {
                    if (collapseCell(x, y, z)) remainingCells++;
                }
            }
        }
    }
    
    std::cout << "3D Grid generation complete. Processed " << remainingCells << " remaining cells." << std::endl;
}

void BlockGenerator::updateWorldDimensions() {
    parameters.worldWidth = parameters.gridWidth * parameters.cellWidth;
    parameters.worldHeight = parameters.gridHeight * parameters.cellHeight;  // Added world height
    parameters.worldLength = parameters.gridLength * parameters.cellLength;
    parameters.halfWorldWidth = parameters.worldWidth / 2.0f;
    parameters.halfWorldLength = parameters.worldLength / 2.0f;
}

void BlockGenerator::DetectCellSizeFromAssets() {
    if (!controller || !controller->GetBlockUI()) return;

    auto assets = controller->GetBlockUI()->GetLoadedAssets();
    if (assets.empty()) return;
    
    const auto& firstAsset = assets[0];
    if (!firstAsset.model) return;
    
    glm::vec3 bounds = calculateModelBounds(firstAsset.model);
    parameters.detectedBlockWidth = bounds.x;
    parameters.detectedBlockHeight = bounds.y;    // Added height detection
    parameters.detectedBlockLength = bounds.z;
    parameters.dimensionsDetected = true;
    parameters.cellWidth = bounds.x;
    parameters.cellHeight = bounds.y;             // Added cell height
    parameters.cellLength = bounds.z;
    
    updateWorldDimensions();
}

glm::vec3 BlockGenerator::calculateModelBounds(const std::shared_ptr<Model>& model) {
    if (!model || model->getMeshes().empty()) {
        return glm::vec3(5.0f);
    }
    
    glm::vec3 minBounds(FLT_MAX), maxBounds(-FLT_MAX);
    
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
    grid.assign(parameters.gridWidth, 
                std::vector<std::vector<GridCell>>(parameters.gridHeight,
                                                 std::vector<GridCell>(parameters.gridLength)));
    auto allBlockTypes = getAllBlockTypes();
    
    for (auto& layer : grid) {
        for (auto& row : layer) {
            for (auto& cell : row) {
                cell = {allBlockTypes, false, {}, {}, 0.0f};
            }
        }
    }
}

std::vector<int> BlockGenerator::getAllBlockTypes() {
    std::vector<int> blockTypes;

    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        if (!assets.empty()) {
            for (const auto& asset : assets) {
                blockTypes.push_back(asset.id);
            }
            return blockTypes;
        }
    }
    
    // Fallback
    for (int i = 0; i < 10; i++) {
        blockTypes.push_back(i);
    }
    return blockTypes;
}

bool BlockGenerator::placeRandomBlockAt(int x, int y, int z) {
    if (!isValidGridPosition(x, y, z)) return false;
    
    auto& cell = grid[x][y][z];
    if (cell.possibleBlockTypes.empty()) return false;
    
    updateCellPossibilities(x, y, z);
    if (cell.possibleBlockTypes.empty()) {
        std::cerr << "No valid blocks for initial placement at (" << x << ", " << y << ", " << z << ")" << std::endl;
        return false;
    }
    
    return collapseCell(x, y, z);
}

GridPosition BlockGenerator::findLowestEntropyCell() {
    int lowestEntropy = INT_MAX;
    std::vector<GridPosition> candidates;
    
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                const auto& cell = grid[x][y][z];
                if (cell.collapsed || cell.possibleBlockTypes.empty()) continue;
                
                int entropy = cell.possibleBlockTypes.size();
                if (entropy < lowestEntropy) {
                    lowestEntropy = entropy;
                    candidates.clear();
                    candidates.push_back({(int)x, (int)y, (int)z});
                } else if (entropy == lowestEntropy) {
                    candidates.push_back({(int)x, (int)y, (int)z});
                }
            }
        }
    }
    
    return candidates.empty() ? GridPosition{-1, -1, -1} : 
           candidates[rand() % candidates.size()];
}

bool BlockGenerator::collapseCell(int x, int y, int z) {
    if (!isValidGridPosition(x, y, z)) return false;
    
    auto& cell = grid[x][y][z];
    if (cell.collapsed || cell.possibleBlockTypes.empty()) return false;
    
    updateCellPossibilities(x, y, z);
    if (cell.possibleBlockTypes.empty()) {
        std::cerr << "Warning: No valid blocks remaining for position (" << x << ", " << y << ", " << z << ")" << std::endl;
        return false;
    }
    
    // Choose and place block
    int chosenBlockType = cell.possibleBlockTypes[rand() % cell.possibleBlockTypes.size()];
    glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
    
    // Collapse cell
    cell.collapsed = true;
    cell.possibleBlockTypes = {chosenBlockType};
    cell.blockTypeIds = {chosenBlockType};
    cell.blockPositions = {blockPosition};
    
    propagateConstraints(x, y, z);
    return true;
}

void BlockGenerator::updateCellPossibilities(int x, int y, int z) {
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return;
    
    std::vector<int> validBlocks;
    for (int blockId : cell.possibleBlockTypes) {
        if (isBlockValidAtPosition(x, y, z, blockId)) {
            validBlocks.push_back(blockId);
        }
    }
    cell.possibleBlockTypes = validBlocks;
}

bool BlockGenerator::isBlockValidAtPosition(int x, int y, int z, int blockId) {
    const std::vector<std::tuple<int, int, int, std::string>> neighbors = {
        {x, y, z+1, "+Z"}, {x, y, z-1, "-Z"}, 
        {x+1, y, z, "+X"}, {x-1, y, z, "-X"}, 
        {x, y+1, z, "+Y"}, {x, y-1, z, "-Y"}
    };
    
    for (const auto& [nx, ny, nz, direction] : neighbors) {
        if (!isValidGridPosition(nx, ny, nz)) {
            if (!canFaceBeExposed(blockId, direction)) return false;
            continue;
        }
        
        const auto& neighborCell = grid[nx][ny][nz];
        if (!validateNeighborCompatibility(blockId, direction, neighborCell)) {
            return false;
        }
    }
    return true;
}

bool BlockGenerator::validateNeighborCompatibility(int blockId, const std::string& direction, 
                                                  const GridCell& neighborCell) {
    std::string oppositeFace = getOppositeFace(direction);
    
    if (neighborCell.collapsed && !neighborCell.blockTypeIds.empty()) {
        return canBlocksConnectMutually(blockId, direction, neighborCell.blockTypeIds[0], oppositeFace);
    }
    
    if (!neighborCell.collapsed) {
        for (int possibleNeighborId : neighborCell.possibleBlockTypes) {
            if (canBlocksConnectMutually(blockId, direction, possibleNeighborId, oppositeFace)) {
                return true;
            }
        }
        return canFaceBeExposed(blockId, direction);
    }
    
    return canFaceBeExposed(blockId, direction);
}

void BlockGenerator::propagateConstraints(int x, int y, int z) {
    const std::vector<std::tuple<int, int, int>> neighbors = {
        {x, y, z+1}, {x, y, z-1}, 
        {x+1, y, z}, {x-1, y, z}, 
        {x, y+1, z}, {x, y-1, z}
    };
    
    for (const auto& [nx, ny, nz] : neighbors) {
        if (isValidGridPosition(nx, ny, nz) && !grid[nx][ny][nz].collapsed) {
            updateCellPossibilities(nx, ny, nz);
        }
    }
}

BlockMesh* BlockGenerator::generateMeshFromGrid() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;
    std::set<std::shared_ptr<Texture>> uniqueTextures;
    
    BlockMesh* blockMesh = new BlockMesh(vertices, indices, parameters, textures);
    
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                const auto& cell = grid[x][y][z];
                if (!cell.collapsed) continue;
                
                for (size_t i = 0; i < cell.blockTypeIds.size(); i++) {
                    addBlockToMesh(blockMesh, cell.blockTypeIds[i], cell.blockPositions[i], uniqueTextures);
                }
            }
        }
    }
    
    blockMesh->SetBlockTextures({uniqueTextures.begin(), uniqueTextures.end()});
    return blockMesh;
}

void BlockGenerator::ApplyRandomRotationsToGrid() {
    if (!generatorMesh) {
        std::cout << "No world generated yet. Generate a world first!" << std::endl;
        return;
    }
    
    std::cout << "Applying random Y-axis rotations to all blocks..." << std::endl;
    
    // Apply random rotations to all collapsed cells
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                auto& cell = grid[x][y][z];
                if (cell.collapsed && !cell.blockPositions.empty()) {
                    // We don't modify the position, just regenerate the mesh with rotations
                    // The rotation will be applied during mesh generation
                }
            }
        }
    }
    
    // Regenerate mesh with rotations
    if (generatorMesh) {
        delete generatorMesh;
    }
    
    // Temporarily enable random rotations for mesh generation
    bool wasEnabled = randomRotationsEnabled;
    randomRotationsEnabled = true;
    
    generatorMesh = generateMeshFromGrid();
    
    // Restore original setting
    randomRotationsEnabled = wasEnabled;
    
    std::cout << "Random rotations applied successfully!" << std::endl;
}

void BlockGenerator::addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position,
                                   std::set<std::shared_ptr<Texture>>& uniqueTextures) {
    Transform blockTransform;
    blockTransform.setPosition(position);
    blockTransform.setScale(parameters.blockScale);
    
    // Apply random Y-axis rotation if enabled
    if (randomRotationsEnabled) {
        float randomYRotation = getRandomYRotation();
        blockTransform.setRotation(glm::vec3(0.0f, randomYRotation, 0.0f));
    }

    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets) {
            if (asset.id == blockId) {
                blockMesh->AddBlockInstance(asset.blockPath, blockTransform);
                collectTexturesFromModel(asset.model, uniqueTextures);
                return;
            }
        }
    }
    
    blockMesh->AddBlockInstance(blockId, blockTransform);
}

float BlockGenerator::getRandomYRotation() const {
    // Return random rotation in 90-degree increments (0°, 90°, 180°, 270°)
    static const float rotations[] = {0.0f, 90.0f, 180.0f, 270.0f};
    int randomIndex = rand() % 4;
    return rotations[randomIndex];
}

void BlockGenerator::collectTexturesFromModel(const std::shared_ptr<Model>& model,
                                             std::set<std::shared_ptr<Texture>>& uniqueTextures) {
    if (!model) return;
    
    for (auto* mesh : model->getMeshes()) {
        if (mesh) {
            for (auto& texture : mesh->textures) {
                uniqueTextures.insert(texture);
            }
        }
    }
}

bool BlockGenerator::canBlocksConnect(int blockId1, const std::string& face1, int blockId2, const std::string& face2) {
    auto it = blockConstraints.find(blockId1);
    if (it == blockConstraints.end()) return true;
    
    const BlockFaceConstraints* faceConstraints = getFaceConstraints(it->second, face1);
    if (!faceConstraints || faceConstraints->validConnections.empty()) return true;
    
    return std::find(faceConstraints->validConnections.begin(), 
                    faceConstraints->validConnections.end(), 
                    blockId2) != faceConstraints->validConnections.end();
}

const BlockFaceConstraints* BlockGenerator::getFaceConstraints(const BlockConstraints& constraints, 
                                                              const std::string& face) {
    static const std::map<std::string, std::function<const BlockFaceConstraints&(const BlockConstraints&)>> faceMap = {
        {"+Z", [](const auto& c) -> const auto& { return c.posZ; }},
        {"-Z", [](const auto& c) -> const auto& { return c.negZ; }},
        {"+X", [](const auto& c) -> const auto& { return c.posX; }},
        {"-X", [](const auto& c) -> const auto& { return c.negX; }},
        {"+Y", [](const auto& c) -> const auto& { return c.posY; }},
        {"-Y", [](const auto& c) -> const auto& { return c.negY; }}
    };
    
    auto it = faceMap.find(face);
    return it != faceMap.end() ? &it->second(constraints) : nullptr;
}

std::string BlockGenerator::getOppositeFace(const std::string& face) {
    static const std::map<std::string, std::string> opposites = {
        {"+Z", "-Z"}, {"-Z", "+Z"}, {"+X", "-X"}, 
        {"-X", "+X"}, {"+Y", "-Y"}, {"-Y", "+Y"}
    };
    
    auto it = opposites.find(face);
    return it != opposites.end() ? it->second : face;
}

bool BlockGenerator::canBlocksConnectMutually(int blockId1, const std::string& face1, 
                                              int blockId2, const std::string& face2) {
    return canBlocksConnect(blockId1, face1, blockId2, face2) && 
           canBlocksConnect(blockId2, face2, blockId1, face1);
}

bool BlockGenerator::canFaceBeExposed(int blockId, const std::string& face) {
    auto it = blockConstraints.find(blockId);
    if (it == blockConstraints.end()) return true;

    const BlockFaceConstraints* faceConstraints = getFaceConstraints(it->second, face);
    return faceConstraints ? faceConstraints->canBeExposed : true;
}

void BlockGenerator::initializeBlockConstraints() {
    blockConstraints.clear();
    
    auto createDefaultConstraints = [](int blockId) {
        return BlockConstraints{blockId, {{}, true}, {{}, true}, {{}, true}, 
                               {{}, true}, {{}, true}, {{}, true}};
    };
    
    if (controller && controller->GetBlockUI()) {
        auto uiConstraints = controller->GetBlockUI()->GetConstraints();
        if (!uiConstraints.empty()) {
            blockConstraints = uiConstraints;
        } else {
            auto assets = controller->GetBlockUI()->GetLoadedAssets();
            for (const auto& asset : assets) {
                blockConstraints[asset.id] = createDefaultConstraints(asset.id);
            }
        }
    } else {
        auto allBlockTypes = getAllBlockTypes();
        for (int blockId : allBlockTypes) {
            blockConstraints[blockId] = createDefaultConstraints(blockId);
        }
    }
    
    std::cout << "Initialized constraints for " << blockConstraints.size() << " block types." << std::endl;
}

// Helper functions
bool BlockGenerator::isValidGridPosition(int x, int y, int z) const {
    return x >= 0 && x < (int)parameters.gridWidth && 
           y >= 0 && y < (int)parameters.gridHeight && 
           z >= 0 && z < (int)parameters.gridLength;
}

glm::vec3 BlockGenerator::calculateBlockPosition(int x, int y, int z) const {
    float cellCenterX = (x * parameters.cellWidth) - parameters.halfWorldWidth;
    float cellCenterY = y * parameters.cellHeight; // Y starts from 0
    float cellCenterZ = (z * parameters.cellLength) - parameters.halfWorldLength;
    return glm::vec3(cellCenterX, cellCenterY, cellCenterZ);
}

BlockMesh* BlockGenerator::createEmptyMesh() {
    return new BlockMesh(std::vector<Vertex>(), std::vector<unsigned int>(), parameters);
}


