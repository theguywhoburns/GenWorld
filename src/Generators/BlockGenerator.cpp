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
    
    // Initialize block weights and limits
    initializeBlockWeights();
    resetBlockCounts();
    
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
    
    // Remove building generator initialization - no longer needed
    
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
                
                // Handle void cells properly in animation
                cell.collapsed = true;
                cell.possibleBlockTypes = {chosenBlockType};
                cell.blockTypeIds = {chosenBlockType};
                
                // Only add position if it's not a void cell
                if (chosenBlockType != VOID_BLOCK_ID) {
                    glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
                    cell.blockPositions = {blockPosition};
                } else {
                    cell.blockPositions.clear();
                }
                
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
    parameters.worldHeight = parameters.gridHeight * parameters.cellHeight;
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
    parameters.detectedBlockHeight = bounds.y;
    parameters.detectedBlockLength = bounds.z;
    parameters.dimensionsDetected = true;
    parameters.cellWidth = bounds.x;
    parameters.cellHeight = bounds.y;
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

    // Add void cell if enabled
    if (parameters.enableVoidCells) {
        blockTypes.push_back(VOID_BLOCK_ID);
    }

    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        if (!assets.empty()) {
            for (const auto& asset : assets) {
                blockTypes.push_back(asset.id);
            }
            return blockTypes;
        }
    }
    
    // Fallback - still include void if enabled
    if (parameters.enableVoidCells) {
        blockTypes.push_back(VOID_BLOCK_ID);
    }
    
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
    
    // Use pure WFC logic with weighted selection
    updateCellPossibilities(x, y, z);
    if (cell.possibleBlockTypes.empty()) {
        std::cerr << "Warning: No valid blocks remaining for position (" << x << ", " << y << ", " << z << ")" << std::endl;
        return false;
    }
    
    // Use weighted selection that respects count limits
    int chosenBlockType = selectWeightedBlock(cell.possibleBlockTypes);
    
    // If no block can be placed due to count limits, place void or skip
    if (chosenBlockType == -1) {
        std::cout << "No blocks available due to count limits at (" << x << ", " << y << ", " << z << ")" << std::endl;
        // You can either place a void block or leave the cell uncollapsed
        if (parameters.enableVoidCells) {
            chosenBlockType = VOID_BLOCK_ID;
        } else {
            return false; // Skip this cell
        }
    }
    
    // Collapse cell
    cell.collapsed = true;
    cell.possibleBlockTypes = {chosenBlockType};
    cell.blockTypeIds = {chosenBlockType};
    
    // Increment block count (only for non-void blocks)
    if (chosenBlockType != VOID_BLOCK_ID) {
        incrementBlockCount(chosenBlockType);
    }
    
    // Only add position if it's not a void cell
    if (chosenBlockType != VOID_BLOCK_ID) {
        glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
        cell.blockPositions = {blockPosition};
    } else {
        cell.blockPositions.clear();
    }
    
    propagateConstraints(x, y, z);
    return true;
}

void BlockGenerator::updateCellPossibilities(int x, int y, int z) {
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return;
    
    std::vector<int> validBlocks;
    for (int blockId : cell.possibleBlockTypes) {
        if (isBlockValidAtPosition(x, y, z, blockId) && canPlaceBlock(blockId)) {
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
            // Check if this block can be exposed to air on this face
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
        int neighborBlockId = neighborCell.blockTypeIds[0];
        
        // Special handling for void cells
        if (blockId == VOID_BLOCK_ID || neighborBlockId == VOID_BLOCK_ID) {
            return canBlocksConnectMutually(blockId, direction, neighborBlockId, oppositeFace);
        }
        
        return canBlocksConnectMutually(blockId, direction, neighborBlockId, oppositeFace);
    }
    
    if (!neighborCell.collapsed) {
        // Check if any of the possible neighbor blocks can connect
        bool canConnect = false;
        for (int possibleNeighborId : neighborCell.possibleBlockTypes) {
            if (canBlocksConnectMutually(blockId, direction, possibleNeighborId, oppositeFace)) {
                canConnect = true;
                break;
            }
        }
        
        if (canConnect) return true;
        
        // If no blocks can connect, check if this face can be exposed
        return canFaceBeExposed(blockId, direction);
    }
    
    return canFaceBeExposed(blockId, direction);
}

bool BlockGenerator::canBlocksConnect(int blockId1, const std::string& face1, int blockId2, const std::string& face2) {
    // Special case: void cells have their own constraint logic
    if (blockId1 == VOID_BLOCK_ID || blockId2 == VOID_BLOCK_ID) {
        auto it1 = blockConstraints.find(blockId1);
        if (it1 == blockConstraints.end()) return true; // No constraints = allow everything
        
        const BlockFaceConstraints* faceConstraints = getFaceConstraints(it1->second, face1);
        if (!faceConstraints || faceConstraints->validConnections.empty()) return true;
        
        return std::find(faceConstraints->validConnections.begin(), 
                        faceConstraints->validConnections.end(), 
                        blockId2) != faceConstraints->validConnections.end();
    }
    
    // Regular block-to-block connection logic
    auto it = blockConstraints.find(blockId1);
    if (it == blockConstraints.end()) return true;
    
    const BlockFaceConstraints* faceConstraints = getFaceConstraints(it->second, face1);
    if (!faceConstraints || faceConstraints->validConnections.empty()) return true;
    
    return std::find(faceConstraints->validConnections.begin(), 
                    faceConstraints->validConnections.end(), 
                    blockId2) != faceConstraints->validConnections.end();
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
                    // Skip void cells in mesh generation
                    if (cell.blockTypeIds[i] == VOID_BLOCK_ID) continue;
                    
                    if (i < cell.blockPositions.size()) {
                        addBlockToMesh(blockMesh, cell.blockTypeIds[i], cell.blockPositions[i], uniqueTextures);
                    }
                }
            }
        }
    }
    
    blockMesh->SetBlockTextures({uniqueTextures.begin(), uniqueTextures.end()});
    return blockMesh;
}

void BlockGenerator::addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position,
                                   std::set<std::shared_ptr<Texture>>& uniqueTextures) {
    // Skip void cells - they don't get added to the mesh
    if (blockId == VOID_BLOCK_ID) {
        return;
    }
    
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

void BlockGenerator::ApplyRandomRotationsToGrid() {
    if (!generatorMesh) {
        std::cout << "No world generated yet. Generate a world first!" << std::endl;
        return;
    }
    
    std::cout << "Applying random Y-axis rotations to all blocks..." << std::endl;
    
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

float BlockGenerator::getRandomYRotation() const {
    // Return random rotation in 90-degree increments (0°, 90°, 180°, 270°)
    static const float rotations[] = {0.0f, 90.0f, 180.0f, 270.0f};
    int randomIndex = rand() % 4;
    return rotations[randomIndex];
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
    
    // Create constraints for void cells
    if (parameters.enableVoidCells) {
        BlockConstraints voidConstraints = createDefaultConstraints(VOID_BLOCK_ID);
        // Void cells can connect to anything and can always be exposed
        voidConstraints.posZ.canBeExposed = true;
        voidConstraints.negZ.canBeExposed = true;
        voidConstraints.posX.canBeExposed = true;
        voidConstraints.negX.canBeExposed = true;
        voidConstraints.posY.canBeExposed = true;
        voidConstraints.negY.canBeExposed = true;
        blockConstraints[VOID_BLOCK_ID] = voidConstraints;
    }
    
    if (controller && controller->GetBlockUI()) {
        auto uiConstraints = controller->GetBlockUI()->GetConstraints();
        if (!uiConstraints.empty()) {
            // Merge UI constraints with void constraints
            for (const auto& [blockId, constraints] : uiConstraints) {
                blockConstraints[blockId] = constraints;
            }
        } else {
            auto assets = controller->GetBlockUI()->GetLoadedAssets();
            for (const auto& asset : assets) {
                blockConstraints[asset.id] = createDefaultConstraints(asset.id);
            }
        }
    } else {
        auto allBlockTypes = getAllBlockTypes();
        for (int blockId : allBlockTypes) {
            if (blockId != VOID_BLOCK_ID) { // Already handled above
                blockConstraints[blockId] = createDefaultConstraints(blockId);
            }
        }
    }
    
    std::cout << "Initialized constraints for " << blockConstraints.size() << " block types (including void cells)." << std::endl;
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

void BlockGenerator::initializeBlockWeights() {
    auto& settings = parameters.generationSettings;
    
    // Reset counters
    settings.currentBlockCounts.clear();
    
    std::cout << "Initializing block weights..." << std::endl;
    
    // Initialize weights and limits for all available blocks
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets) {
            // Set default weight if not already set
            if (settings.blockWeights.find(asset.id) == settings.blockWeights.end()) {
                settings.blockWeights[asset.id] = settings.defaultWeight;
            }
            
            // DON'T override existing limits from UI - only set if not found
            if (settings.maxBlockCounts.find(asset.id) == settings.maxBlockCounts.end()) {
                settings.maxBlockCounts[asset.id] = -1; // Unlimited by default
            }
            
            // Initialize counter to 0
            settings.currentBlockCounts[asset.id] = 0;
            
            std::cout << "  Block " << asset.id << ": weight=" << settings.blockWeights[asset.id] 
                      << ", limit=" << settings.maxBlockCounts[asset.id] << std::endl;
        }
    }
    
    // Initialize void block settings
    if (parameters.enableVoidCells) {
        if (settings.blockWeights.find(VOID_BLOCK_ID) == settings.blockWeights.end()) {
            settings.blockWeights[VOID_BLOCK_ID] = parameters.voidProbability;
        }
        
        if (settings.maxBlockCounts.find(VOID_BLOCK_ID) == settings.maxBlockCounts.end()) {
            settings.maxBlockCounts[VOID_BLOCK_ID] = -1; // Unlimited by default
        }
        
        settings.currentBlockCounts[VOID_BLOCK_ID] = 0;
    }
    
    std::cout << "Initialized block weights and limits for " << settings.blockWeights.size() << " block types" << std::endl;
}

void BlockGenerator::resetBlockCounts() {
    auto& settings = parameters.generationSettings;
    for (auto& [blockId, count] : settings.currentBlockCounts) {
        count = 0;
    }
    std::cout << "Reset all block counts to 0" << std::endl;
}

bool BlockGenerator::canPlaceBlock(int blockId) const {
    auto& settings = parameters.generationSettings;
    
    // Use find() instead of operator[] for const method
    auto maxIt = settings.maxBlockCounts.find(blockId);
    auto currentIt = settings.currentBlockCounts.find(blockId);
    
    // Get current count (default to 0 if not found)
    int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
    
    // Check if unlimited (maxCount == -1 or not found)
    bool isUnlimited = (maxIt == settings.maxBlockCounts.end() || maxIt->second == -1);
    
    if (isUnlimited) {
        return true; // Unlimited blocks can always be placed
    } else {
        int maxCount = maxIt->second;
        bool canPlace = currentCount < maxCount;
        
        if (!canPlace) {
            std::cout << "Block " << blockId << " has reached limit: " << currentCount << "/" << maxCount << std::endl;
        }
        
        return canPlace;
    }
}

int BlockGenerator::selectBlockType() {
    auto& settings = parameters.generationSettings;
    
    // First, check if we should use count-based selection
    std::vector<int> availableBlocks;
    
    for (const auto& asset : loadedAssets) {
        if (canPlaceBlock(asset.id)) {
            availableBlocks.push_back(asset.id);
        }
    }
    
    if (availableBlocks.empty()) {
        return VOID_BLOCK_ID; // No blocks available, place void
    }
    
    // If all remaining blocks are unlimited, use weight-based selection
    bool allUnlimited = true;
    for (int blockId : availableBlocks) {
        if (settings.maxBlockCounts[blockId] != -1) {
            allUnlimited = false;
            break;
        }
    }
    
    if (allUnlimited) {
        // Use weight-based selection among unlimited blocks
        return selectByWeight(availableBlocks);
    } else {
        // Prioritize count-based blocks (non-unlimited) first
        std::vector<int> countBasedBlocks;
        for (int blockId : availableBlocks) {
            if (settings.maxBlockCounts[blockId] != -1) {
                countBasedBlocks.push_back(blockId);
            }
        }
        
        if (!countBasedBlocks.empty()) {
            // Randomly select from count-based blocks
            return countBasedBlocks[rand() % countBasedBlocks.size()];
        } else {
            // Fall back to weight-based selection
            return selectByWeight(availableBlocks);
        }
    }
}

int BlockGenerator::selectByWeight(const std::vector<int>& blocks) {
    if (blocks.empty()) return VOID_BLOCK_ID;
    
    auto& settings = parameters.generationSettings;
    
    // Calculate total weight
    float totalWeight = 0.0f;
    for (int blockId : blocks) {
        auto it = settings.blockWeights.find(blockId);
        if (it != settings.blockWeights.end()) {
            totalWeight += it->second;
        }
    }
    
    if (totalWeight <= 0.0f) {
        // No weights set, use random selection
        return blocks[rand() % blocks.size()];
    }
    
    // Weighted random selection
    float randomValue = (rand() / (float)RAND_MAX) * totalWeight;
    float currentWeight = 0.0f;
    
    for (int blockId : blocks) {
        auto it = settings.blockWeights.find(blockId);
        if (it != settings.blockWeights.end()) {
            currentWeight += it->second;
            if (randomValue <= currentWeight) {
                return blockId;
            }
        }
    }
    
    // Fallback to last block
    return blocks.back();
}

bool BlockGenerator::isUnlimitedBlock(int blockId) const {
    auto& settings = parameters.generationSettings;
    auto it = settings.maxBlockCounts.find(blockId);
    return (it == settings.maxBlockCounts.end() || it->second == -1);
}

std::vector<int> BlockGenerator::getAvailableBlocks() const {
    std::vector<int> available;
    
    // Get blocks from UI if available
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets) {
            if (canPlaceBlock(asset.id)) {
                available.push_back(asset.id);
            }
        }
    }
    
    return available;
}

bool BlockGenerator::hasReachedLimit(int blockId) const {
    auto& settings = parameters.generationSettings;
    
    auto maxIt = settings.maxBlockCounts.find(blockId);
    auto currentIt = settings.currentBlockCounts.find(blockId);
    
    if (maxIt == settings.maxBlockCounts.end() || maxIt->second == -1) {
        return false; // Unlimited
    }
    
    int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
    return currentCount >= maxIt->second;
}

void BlockGenerator::incrementBlockCount(int blockId) {
    auto& settings = parameters.generationSettings;
    settings.currentBlockCounts[blockId]++;
    
    std::cout << "Block " << blockId << " count incremented to: " 
              << settings.currentBlockCounts[blockId] << std::endl;
}

// Update your selectWeightedBlock method in BlockGenerator.cpp
int BlockGenerator::selectWeightedBlock(const std::vector<int>& validBlocks) {
    // Filter blocks that can still be placed (haven't reached their count limit)
    std::vector<int> availableBlocks;
    for (int blockId : validBlocks) {
        if (canPlaceBlock(blockId)) {
            availableBlocks.push_back(blockId);
        }
    }
    
    // If no blocks are available due to count limits, return VOID_BLOCK_ID or -1
    if (availableBlocks.empty()) {
        std::cout << "No blocks available - all have reached their count limits!" << std::endl;
        return VOID_BLOCK_ID; // Or return -1 to indicate no block should be placed
    }
    
    // Use weight-based selection only on blocks that can still be placed
    return selectByWeight(availableBlocks);
}
