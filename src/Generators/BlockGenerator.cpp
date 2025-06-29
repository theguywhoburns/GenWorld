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
#include <future>

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
    // NEW: Initialize socket system instead of old constraints
    initializeSocketSystem();
    
    // Initialize block weights and limits
    initializeBlockWeights();
    resetBlockCounts();

    
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

    generateGridMultithreaded();
    

    generatorMesh = generateMeshFromGrid();
}

void BlockGenerator::initializeSocketSystem() {
    parameters.socketSystem.Initialize();
    
    // Create default templates for all loaded assets
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets) {
            BlockTemplate blockTemplate(asset.id);
            blockTemplate.name = asset.name;
            
            // Set all sockets to EMPTY by default (connects to everything)
            for (int i = 0; i < 6; i++) {
                blockTemplate.sockets[i] = Socket(SocketType::EMPTY, true);
            }
            
            parameters.socketSystem.AddBlockTemplate(blockTemplate);
            
            // Initialize rotation to 0
            parameters.blockRotations[asset.id] = 0;
        }
    }
    
    // Create void block template
    if (parameters.enableVoidCells) {
        BlockTemplate voidTemplate(VOID_BLOCK_ID);
        voidTemplate.name = "Void";
        // Void has EMPTY sockets (can connect to anything)
        for (int i = 0; i < 6; i++) {
            voidTemplate.sockets[i] = Socket(SocketType::EMPTY, true);
        }
        parameters.socketSystem.AddBlockTemplate(voidTemplate);
        parameters.blockRotations[VOID_BLOCK_ID] = 0;
    }
    
    parameters.socketSystem.GenerateRotatedVariants();
    std::cout << "Socket system initialized for all block types" << std::endl;
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
                cell = {allBlockTypes, false, {}, {}, {}, 0.0f}; // Added {} for blockRotations
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
    std::mutex mutex;

    unsigned int numThreads = std::min(4u, std::thread::hardware_concurrency());
    std::vector<std::future<void>> futures;

    // Lambda for each thread to process a chunk of the grid
    auto processChunk = [&](unsigned int startX, unsigned int endX) {
        int localLowestEntropy = INT_MAX;
        std::vector<GridPosition> localCandidates;

        for (unsigned int x = startX; x < endX; x++) {
            for (unsigned int y = 0; y < parameters.gridHeight; y++) {
                for (unsigned int z = 0; z < parameters.gridLength; z++) {
                    const auto& cell = grid[x][y][z];
                    if (cell.collapsed || cell.possibleBlockTypes.empty()) continue;

                    int entropy = cell.possibleBlockTypes.size();
                    if (entropy < localLowestEntropy) {
                        localLowestEntropy = entropy;
                        localCandidates.clear();
                        localCandidates.push_back({(int)x, (int)y, (int)z});
                    } else if (entropy == localLowestEntropy) {
                        localCandidates.push_back({(int)x, (int)y, (int)z});
                    }
                }
            }
        }

        std::lock_guard<std::mutex> lock(mutex);
        if (localLowestEntropy < lowestEntropy) {
            lowestEntropy = localLowestEntropy;
            candidates = std::move(localCandidates);
        } else if (localLowestEntropy == lowestEntropy) {
            candidates.insert(candidates.end(), localCandidates.begin(), localCandidates.end());
        }
    };

    unsigned int chunkSize = parameters.gridWidth / numThreads;
    unsigned int remainder = parameters.gridWidth % numThreads;
    unsigned int startX = 0;

    for (unsigned int t = 0; t < numThreads; ++t) {
        unsigned int endX = startX + chunkSize + (t < remainder ? 1 : 0);
        futures.push_back(std::async(std::launch::async, processChunk, startX, endX));
        startX = endX;
    }

    for (auto& f : futures) {
        f.get();
    }

    if (candidates.empty()) {
        return GridPosition{-1, -1, -1};
    }
    return candidates[rand() % candidates.size()];
}

bool BlockGenerator::collapseCell(int x, int y, int z) {
    if (!isValidGridPosition(x, y, z)) return false;

    auto& cell = grid[x][y][z];
    if (cell.collapsed) return false;

    updateCellPossibilities(x, y, z);

    // If no valid (block, rotation) pairs, set to void/empty
    if (cell.possibleBlockRotationPairs.empty()) {
        cell.collapsed = true;
        cell.possibleBlockTypes = {VOID_BLOCK_ID};
        cell.blockTypeIds = {VOID_BLOCK_ID};
        cell.blockRotations = {0};
        cell.blockPositions.clear();
        return true;
    }

    // Instead of picking a random pair:
    std::vector<int> blockCandidates;
    for (const auto& pair : cell.possibleBlockRotationPairs) {
        blockCandidates.push_back(pair.first);
    }
    int chosenBlockType = selectWeightedBlock(blockCandidates);

    // Now pick a rotation for the chosen block (random or weighted if you want)
    std::vector<int> rotations;
    for (const auto& pair : cell.possibleBlockRotationPairs) {
        if (pair.first == chosenBlockType) {
            rotations.push_back(pair.second);
        }
    }
    int chosenRotation = rotations.empty() ? 0 : rotations[rand() % rotations.size()];

    
    cell.collapsed = true;
    cell.possibleBlockTypes = {chosenBlockType};
    cell.blockTypeIds = {chosenBlockType};
    cell.blockRotations = {chosenRotation};

    if (chosenBlockType != VOID_BLOCK_ID) {
        incrementBlockCount(chosenBlockType);
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
    std::vector<std::pair<int, int>> validPairs;

    for (int blockId : cell.possibleBlockTypes) {
        if (!canPlaceBlock(blockId)) continue;

        auto& templates = parameters.socketSystem.GetBlockTemplates();
        auto it = templates.find(blockId);

        bool foundValidRotation = false;
        if (it != templates.end()) {
            for (int rotation : it->second.allowedRotations) {
                if (isBlockValidAtPosition(x, y, z, blockId, rotation)) {
                    validPairs.emplace_back(blockId, rotation);
                    foundValidRotation = true;
                }
            }
        } else {
            if (isBlockValidAtPosition(x, y, z, blockId, 0)) {
                validPairs.emplace_back(blockId, 0);
                foundValidRotation = true;
            }
        }
        if (foundValidRotation) {
            validBlocks.push_back(blockId);
        }
    }

    cell.possibleBlockTypes = validBlocks;
    cell.possibleBlockRotationPairs = validPairs;
}

// NEW: Socket-based validation
bool BlockGenerator::isBlockValidAtPosition(int x, int y, int z, int blockId, int rotation) {
    const std::vector<std::tuple<int, int, int, int>> neighbors = {
        {x+1, y, z, 0}, // +X
        {x-1, y, z, 1}, // -X  
        {x, y+1, z, 2}, // +Y
        {x, y-1, z, 3}, // -Y
        {x, y, z+1, 4}, // +Z
        {x, y, z-1, 5}  // -Z
    };
    
    for (const auto& [nx, ny, nz, faceIndex] : neighbors) {
        if (!isValidGridPosition(nx, ny, nz)) {
            // At world boundary - always allow for now
            continue;
        }
        
        const auto& neighborCell = grid[nx][ny][nz];
        if (!validateNeighborCompatibility(blockId, rotation, faceIndex, neighborCell, nx, ny, nz)) {
            return false;
        }
    }
    return true;
}

bool BlockGenerator::validateNeighborCompatibility(int blockId, int rotation, int faceIndex, 
                                                  const GridCell& neighborCell, int neighborX, int neighborY, int neighborZ) {
    int oppositeFace = getOppositeFaceIndex(faceIndex);
    
    if (neighborCell.collapsed && !neighborCell.blockTypeIds.empty()) {
        // Neighbor is already placed
        int neighborBlockId = neighborCell.blockTypeIds[0];
        int neighborRotation = neighborCell.blockRotations.empty() ? 0 : neighborCell.blockRotations[0];
        
        return canBlocksConnectWithSockets(blockId, rotation, faceIndex, 
                                          neighborBlockId, neighborRotation, oppositeFace);
    }
    
    if (!neighborCell.collapsed) {
        // Check if any possible neighbor configuration can connect
        for (int possibleNeighborId : neighborCell.possibleBlockTypes) {
            // Try all possible rotations for the neighbor
            auto& socketSystem = parameters.socketSystem;
            auto& templates = socketSystem.GetBlockTemplates();
            auto it = templates.find(possibleNeighborId);
            
            if (it != templates.end()) {
                for (int neighborRotation : it->second.allowedRotations) {
                    if (canBlocksConnectWithSockets(blockId, rotation, faceIndex, 
                                                   possibleNeighborId, neighborRotation, oppositeFace)) {
                        return true; // Found at least one valid connection
                    }
                }
            }
        }
        return false;
    }
    
    return true;
}

bool BlockGenerator::canBlocksConnectWithSockets(int blockId1, int rotation1, int face1, 
                                                 int blockId2, int rotation2, int face2) {
    return parameters.socketSystem.CanBlocksConnect(blockId1, rotation1, face1, 
                                                   blockId2, rotation2, face2);
}

int BlockGenerator::getFaceIndex(const std::string& faceDirection) {
    static const std::map<std::string, int> faceMap = {
        {"+X", 0}, {"-X", 1}, {"+Y", 2}, {"-Y", 3}, {"+Z", 4}, {"-Z", 5}
    };
    
    auto it = faceMap.find(faceDirection);
    return it != faceMap.end() ? it->second : 0;
}

std::string BlockGenerator::getFaceDirection(int faceIndex) {
    static const std::vector<std::string> directions = {
        "+X", "-X", "+Y", "-Y", "+Z", "-Z"
    };
    
    return (faceIndex >= 0 && faceIndex < 6) ? directions[faceIndex] : "+X";
}

int BlockGenerator::getOppositeFaceIndex(int faceIndex) {
    // Opposite face mappings: +X↔-X, +Y↔-Y, +Z↔-Z
    static const std::vector<int> opposites = {1, 0, 3, 2, 5, 4};
    return (faceIndex >= 0 && faceIndex < 6) ? opposites[faceIndex] : 0;
}

int BlockGenerator::getRandomRotationIndex() const {
    // Return 0, 1, 2, or 3 representing 0°, 90°, 180°, 270°
    return rand() % 4;
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

    // Multithreaded mesh collection
    unsigned int numThreads = std::min(4u, std::thread::hardware_concurrency());
    std::vector<std::vector<std::tuple<int, glm::vec3, int>>> threadBlocks(numThreads);
    std::vector<std::set<std::shared_ptr<Texture>>> threadTextures(numThreads);

    auto meshWorker = [&](unsigned int threadId, unsigned int startX, unsigned int endX) {
        for (unsigned int x = startX; x < endX; x++) {
            for (unsigned int y = 0; y < parameters.gridHeight; y++) {
                for (unsigned int z = 0; z < parameters.gridLength; z++) {
                    const auto& cell = grid[x][y][z];
                    if (!cell.collapsed) continue;

                    for (size_t i = 0; i < cell.blockTypeIds.size(); i++) {
                        if (cell.blockTypeIds[i] == VOID_BLOCK_ID) continue;
                        if (i < cell.blockPositions.size()) {
                            int rotation = (i < cell.blockRotations.size()) ? cell.blockRotations[i] : 0;
                            // Store block info for later (blockId, position, rotation)
                            threadBlocks[threadId].emplace_back(cell.blockTypeIds[i], cell.blockPositions[i], rotation);

                            // Collect textures (thread-local)
                            if (controller && controller->GetBlockUI()) {
                                auto assets = controller->GetBlockUI()->GetLoadedAssets();
                                for (const auto& asset : assets) {
                                    if (asset.id == cell.blockTypeIds[i]) {
                                        if (asset.model) {
                                            for (auto* mesh : asset.model->getMeshes()) {
                                                if (mesh) {
                                                    for (auto& texture : mesh->textures) {
                                                        threadTextures[threadId].insert(texture);
                                                    }
                                                }
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    // Divide work along X axis
    std::vector<std::thread> workers;
    unsigned int chunkSize = parameters.gridWidth / numThreads;
    unsigned int remainder = parameters.gridWidth % numThreads;
    unsigned int startX = 0;
    for (unsigned int t = 0; t < numThreads; ++t) {
        unsigned int endX = startX + chunkSize + (t < remainder ? 1 : 0);
        workers.emplace_back(meshWorker, t, startX, endX);
        startX = endX;
    }
    for (auto& th : workers) th.join();

    // Merge results
    for (unsigned int t = 0; t < numThreads; ++t) {
        for (const auto& block : threadBlocks[t]) {
            int blockId;
            glm::vec3 pos;
            int rot;
            std::tie(blockId, pos, rot) = block;
            addBlockToMesh(blockMesh, blockId, pos, rot, uniqueTextures);
        }
        uniqueTextures.insert(threadTextures[t].begin(), threadTextures[t].end());
    }

    blockMesh->SetBlockTextures({uniqueTextures.begin(), uniqueTextures.end()});
    return blockMesh;
}

void BlockGenerator::addBlockToMesh(BlockMesh* blockMesh, int blockId, const glm::vec3& position, int rotation,
                                   std::set<std::shared_ptr<Texture>>& uniqueTextures) {
    // Skip void cells - they don't get added to the mesh
    if (blockId == VOID_BLOCK_ID) {
        return;
    }
    
    Transform blockTransform;
    blockTransform.setPosition(position);
    blockTransform.setScale(parameters.blockScale);
    
    // Apply rotation (convert rotation to degrees)
    float rotationDegrees = static_cast<float>(rotation);
    blockTransform.setRotation(glm::vec3(0.0f, rotationDegrees, 0.0f));

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
    
    // Update rotations in grid
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                auto& cell = grid[x][y][z];
                if (cell.collapsed && !cell.blockTypeIds.empty() && cell.blockTypeIds[0] != VOID_BLOCK_ID) {
                    // Apply random rotation
                    int randomRotation = getRandomRotationIndex() * 90; // 0, 90, 180, or 270 degrees
                    cell.blockRotations = {randomRotation};
                }
            }
        }
    }
    
    // Regenerate mesh with new rotations
    if (generatorMesh) {
        delete generatorMesh;
    }
    
    generatorMesh = generateMeshFromGrid();
    
    std::cout << "Random rotations applied successfully!" << std::endl;
}

float BlockGenerator::getRandomYRotation() const {
    // Return random rotation in 90-degree increments (0°, 90°, 180°, 270°)
    static const float rotations[] = {0.0f, 90.0f, 180.0f, 270.0f};
    int randomIndex = rand() % 4;
    return rotations[randomIndex];
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
    
}

void BlockGenerator::resetBlockCounts() {
    auto& settings = parameters.generationSettings;
    for (auto& [blockId, count] : settings.currentBlockCounts) {
        count = 0;
    }
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
        return true;
    } else {
        int maxCount = maxIt->second;
        bool canPlace = currentCount < maxCount;
             
        return canPlace;
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
}

int BlockGenerator::selectWeightedBlock(const std::vector<int>& validBlocks) {
    // Filter blocks that can still be placed (haven't reached their count limit)
    std::vector<int> availableBlocks;
    for (int blockId : validBlocks) {
        if (canPlaceBlock(blockId)) {
            availableBlocks.push_back(blockId);
        }
    }
    
    // If no blocks are available due to count limits, return VOID_BLOCK_ID
    if (availableBlocks.empty()) {
        std::cout << "No blocks available - all have reached their count limits!" << std::endl;
        return VOID_BLOCK_ID;
    }
    
    // Use weight-based selection only on blocks that can still be placed
    return selectByWeight(availableBlocks);
}
