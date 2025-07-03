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
#include <queue>
#include <tuple>
std::mutex gridMutex;

BlockGenerator::BlockGenerator(BlockController* controller) : controller(controller) { 
    initializeDefaults(); 
}
BlockGenerator::BlockGenerator() : controller(nullptr) { 
    initializeDefaults(); 
}
BlockGenerator::~BlockGenerator() { delete generatorMesh; }

void BlockGenerator::initializeDefaults() {
    parameters = {20, 10, 20, 5.0f, 5.0f, 5.0f, 1.0f};
    updateWorldDimensions();
    generatorMesh = nullptr;
    
    // Initialize grid mask if needed
    if (parameters.generationSettings.isGridMaskEnabled) {
        initializeGridMask();
    }
}

void BlockGenerator::Generate() {
    std::mt19937 mainRng(parameters.randomSeed);
    if (!(controller && controller->GetBlockUI() && !controller->GetBlockUI()->GetLoadedAssets().empty())) {
        std::cerr << "ERROR: No blocks/models loaded. Generation aborted." << std::endl;
        generatorMesh = createEmptyMesh();
        return;
    }
    isFirstBlock = true;
    initializeSocketSystem();
    initializeBlockWeights();
    resetBlockCounts();
    if (!parameters.dimensionsDetected) DetectCellSizeFromAssets();
    initializeGrid();

    // Check if rectangular castle generation is enabled
    if (parameters.generationSettings.isGridMaskEnabled) {
        generateRectangularCastle(mainRng);
    } else {
        generateGridFrontierWFC(mainRng);
    }

    generatorMesh = generateMeshFromGrid();
}

void BlockGenerator::initializeSocketSystem() {
    parameters.socketSystem.Initialize();
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        auto& templates = parameters.socketSystem.GetBlockTemplates();
        for (const auto& asset : assets) {
            if (templates.find(asset.id) == templates.end()) {
                BlockTemplate blockTemplate(asset.id);
                blockTemplate.name = asset.name;
                parameters.socketSystem.AddBlockTemplate(blockTemplate);
            }
            parameters.blockRotations[asset.id] = 0;
        }
    }
    parameters.socketSystem.GenerateRotatedVariants();
    buildAdjacencyTable();
}


void BlockGenerator::generateGridFrontierWFC(std::mt19937& rng) {  
    std::uniform_int_distribution<int> distX(0, parameters.gridWidth - 1);
    std::uniform_int_distribution<int> distY(0, parameters.gridHeight - 1);
    std::uniform_int_distribution<int> distZ(0, parameters.gridLength - 1);
    int centerX = distX(rng);
    int centerY = distY(rng);
    int centerZ = distZ(rng);

    if (!placeRandomBlockAt(centerX, centerY, centerZ, rng)) {
        std::cerr << "Failed to place initial block" << std::endl;
        return;
    }

    std::priority_queue<FrontierCell, std::vector<FrontierCell>, std::greater<>> frontier;
    std::set<GridPosition> inFrontier;

    // Add all neighbors of the initial cell to the frontier
    const std::vector<std::tuple<int, int, int>> neighborOffsets = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    for (const auto& [dx, dy, dz] : neighborOffsets) {
        int nx = centerX + dx, ny = centerY + dy, nz = centerZ + dz;
        if (isValidGridPosition(nx, ny, nz) && !grid[nx][ny][nz].collapsed) {
            // Skip masked cells if grid mask is enabled
            if (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(nx, ny, nz)) {
                continue;
            }
            double entropy = calculateCellEntropy(grid[nx][ny][nz]);
            frontier.push({entropy, {nx, ny, nz}});
            inFrontier.insert({nx, ny, nz});
        }
    }

    // Main loop: process the frontier
    while (!frontier.empty()) {
        // Always pop the lowest-entropy cell
        FrontierCell fc = frontier.top();
        frontier.pop();
        auto& cell = grid[fc.pos.x][fc.pos.y][fc.pos.z];
        if (cell.collapsed || cell.possibleBlockRotationPairs.empty()) {
            inFrontier.erase(fc.pos);
            continue;
        }

        // Collapse the best cell
        collapseCell(fc.pos.x, fc.pos.y, fc.pos.z, rng);
        inFrontier.erase(fc.pos);

        // Add its neighbors to the frontier if not already collapsed or in frontier
        for (const auto& [dx, dy, dz] : neighborOffsets) {
            int nx = fc.pos.x + dx, ny = fc.pos.y + dy, nz = fc.pos.z + dz;
            GridPosition npos{nx, ny, nz};
            if (isValidGridPosition(nx, ny, nz) && !grid[nx][ny][nz].collapsed && inFrontier.find(npos) == inFrontier.end()) {
                // Skip masked cells if grid mask is enabled
                if (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(nx, ny, nz)) {
                    continue;
                }
                double entropy = calculateCellEntropy(grid[nx][ny][nz]);
                frontier.push({entropy, {nx, ny, nz}});
                inFrontier.insert({nx, ny, nz});
            }
        }
    }
}


double BlockGenerator::calculateCellEntropy(const GridCell& cell) const {
    if (cell.collapsed || cell.possibleBlockRotationPairs.empty()) return 0.0;
    double sum = 0.0, logSum = 0.0;
    auto& weights = parameters.generationSettings.blockWeights;
    for (const auto& pair : cell.possibleBlockRotationPairs) {
        int id = pair.first;
        double w = weights.count(id) ? weights.at(id) : 1.0;
        sum += w;
        logSum += w * std::log(w);
    }
    return std::log(sum) - (logSum / sum);
}


bool BlockGenerator::collapseCell(int x, int y, int z, std::mt19937& rng) {
    if (!isValidGridPosition(x, y, z)) return false;
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return false;

    updateCellPossibilities(x, y, z);

    if (cell.possibleBlockRotationPairs.empty()) {
        std::cerr << "Contradiction: No valid block/rotation pairs at (" << x << ", " << y << ", " << z << ")\n";
        cell.collapsed = true;
        cell.possibleBlockRotationPairs.clear();
        cell.blockTypeIds.clear();
        cell.blockRotations.clear();
        cell.blockPositions.clear();
        return false;
    }

    std::uniform_int_distribution<size_t> pairDist(0, cell.possibleBlockRotationPairs.size() - 1);
    auto chosenPair = cell.possibleBlockRotationPairs[pairDist(rng)];
    int chosenBlockType = chosenPair.first;
    int chosenRotation = chosenPair.second;

    cell.collapsed = true;
    cell.possibleBlockRotationPairs = {chosenPair};
    cell.blockTypeIds = {chosenBlockType};
    cell.blockRotations = {chosenRotation};
    glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
    cell.blockPositions = {blockPosition};
    incrementBlockCount(chosenBlockType);

    propagateConstraints(x, y, z);
    return true;
}

int BlockGenerator::selectWeightedBlock(const std::vector<int>& validBlocks, std::mt19937& rng) {
    std::vector<int> availableBlocks;
    for (int blockId : validBlocks)
        if (canPlaceBlock(blockId)) availableBlocks.push_back(blockId);
    if (availableBlocks.empty()) return -1;
    return selectByWeight(availableBlocks, rng);
}

int BlockGenerator::selectByWeight(const std::vector<int>& blocks, std::mt19937& rng) {
    if (blocks.empty()) return -1;
    auto& settings = parameters.generationSettings;
    float totalWeight = 0.0f;
    for (int blockId : blocks) {
        auto it = settings.blockWeights.find(blockId);
        if (it != settings.blockWeights.end()) totalWeight += it->second;
    }
    if (totalWeight <= 0.0f) {
        std::uniform_int_distribution<int> dist(0, blocks.size() - 1);
        return blocks[dist(rng)];
    }
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float randomValue = dist(rng), currentWeight = 0.0f;
    for (int blockId : blocks) {
        auto it = settings.blockWeights.find(blockId);
        if (it != settings.blockWeights.end()) {
            currentWeight += it->second;
            if (randomValue <= currentWeight) return blockId;
        }
    }
    return blocks.back();
}

void BlockGenerator::processRemainingCells(std::mt19937& rng) {
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                if (!grid[x][y][z].collapsed && !grid[x][y][z].possibleBlockRotationPairs.empty())
                    collapseCell(x, y, z, rng);
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
    if (!model || model->getMeshes().empty()) return glm::vec3(5.0f);
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
    grid.assign(parameters.gridWidth, std::vector<std::vector<GridCell>>(parameters.gridHeight, std::vector<GridCell>(parameters.gridLength)));
    auto& templates = parameters.socketSystem.GetBlockTemplates();
    
    for (auto& layer : grid) {
        for (auto& row : layer) {
            for (auto& cell : row) {
                cell = {std::vector<std::pair<int, int>>(), false, {}, {}, {}, 0.0f};
            }
        }
    }
    
    // Initialize possibilities for each cell
    for (int x = 0; x < (int)parameters.gridWidth; ++x) {
        for (int y = 0; y < (int)parameters.gridHeight; ++y) {
            for (int z = 0; z < (int)parameters.gridLength; ++z) {
                std::vector<std::pair<int, int>> cellPairs;
                
                for (const auto& [blockId, blockTemplate] : templates) {
                    // Check if this is a corner block and we're using grid mask
                    bool isCornerBlock = parameters.generationSettings.cornerBlockIds.count(blockId) > 0;
                    if (parameters.generationSettings.isGridMaskEnabled && isCornerBlock) {
                        // Corner blocks can only be placed at corner positions
                        if (!isCornerPosition(x, y, z)) {
                            continue; // Skip this corner block for non-corner positions
                        }
                    }
                    
                    for (int rotation : blockTemplate.allowedRotations) {
                        cellPairs.emplace_back(blockId, rotation);
                    }
                }
                
                grid[x][y][z].possibleBlockRotationPairs = cellPairs;
            }
        }
    }
}

std::vector<int> BlockGenerator::getAllBlockTypes() {
    std::vector<int> blockTypes;
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets) blockTypes.push_back(asset.id);
        return blockTypes;
    }
    for (int i = 0; i < 10; i++) blockTypes.push_back(i);
    return blockTypes;
}

bool BlockGenerator::placeRandomBlockAt(int x, int y, int z, std::mt19937& rng) {
    if (!isValidGridPosition(x, y, z)) return false;
    auto& cell = grid[x][y][z];
    if (cell.possibleBlockRotationPairs.empty()) return false;
    if (isFirstBlock) {
        auto chosenPair = cell.possibleBlockRotationPairs[0];
        int blockId = chosenPair.first;
        int rotation = chosenPair.second;
        cell.collapsed = true;
        cell.possibleBlockRotationPairs = {chosenPair};
        cell.blockTypeIds = {blockId};
        cell.blockRotations = {rotation};
        glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
        cell.blockPositions = {blockPosition};
        incrementBlockCount(blockId);
        propagateConstraints(x, y, z);
        isFirstBlock = false;
        return true;
    }

    if (cell.possibleBlockRotationPairs.empty()) {
        std::cerr << "No valid blocks for initial placement at (" << x << ", " << y << ", " << z << ")" << std::endl;
        return false;
    }
    return collapseCell(x, y, z, rng);
}

void BlockGenerator::updateCellPossibilities(int x, int y, int z) {
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return;
    std::vector<std::pair<int, int>> validPairs;

    auto& templates = parameters.socketSystem.GetBlockTemplates();
    for (const auto& [blockId, blockTemplate] : templates) {
        if (!canPlaceBlock(blockId)) continue;
        
        // Check if this is a corner block and we're using grid mask
        bool isCornerBlock = parameters.generationSettings.cornerBlockIds.count(blockId) > 0;
        if (parameters.generationSettings.isGridMaskEnabled && isCornerBlock) {
            // Corner blocks can only be placed at corner positions
            if (!isCornerPosition(x, y, z)) {
                continue; // Skip this corner block for non-corner positions
            }
        }
        
        for (int rotation : blockTemplate.allowedRotations) {
            if (isBlockValidAtPosition(x, y, z, blockId, rotation)) {
                validPairs.emplace_back(blockId, rotation);
            }
        }
    }
    cell.possibleBlockRotationPairs = validPairs;
}

bool BlockGenerator::isBlockValidAtPosition(int x, int y, int z, int blockId, int rotation) const {
    const std::vector<std::tuple<int, int, int, int>> neighbors = {
        {x+1, y, z, 0}, {x-1, y, z, 1}, {x, y+1, z, 2},
        {x, y-1, z, 3}, {x, y, z+1, 4}, {x, y, z-1, 5}
    };
    for (const auto& [nx, ny, nz, faceIndex] : neighbors) {
        if (!isValidGridPosition(nx, ny, nz)) continue;
        const auto& neighborCell = grid[nx][ny][nz];
        if (!validateNeighborCompatibility(blockId, rotation, faceIndex, neighborCell, nx, ny, nz))
            return false;
    }
    return true;
}

void BlockGenerator::buildAdjacencyTable() {
    adjacencyTable.clear();
    auto& templates = parameters.socketSystem.GetBlockTemplates();
    for (const auto& [blockId, blockTemplate] : templates) {
        for (int rotation : blockTemplate.allowedRotations) {
            for (int face = 0; face < 6; ++face) {
                for (const auto& [neighborId, neighborTemplate] : templates) {
                    for (int neighborRotation : neighborTemplate.allowedRotations) {
                        if (parameters.socketSystem.CanBlocksConnect(
                            blockId, rotation, face,
                            neighborId, neighborRotation, getOppositeFaceIndex(face), false)) {

                            adjacencyTable[{blockId, rotation, face}].insert({neighborId, neighborRotation});
                        }
                    }
                }

                
            }
        }
    }
}

bool BlockGenerator::validateNeighborCompatibility(int blockId, int rotation, int faceIndex,
                                                  const GridCell& neighborCell, int neighborX, int neighborY, int neighborZ) const {
    // Special case for corner blocks: if the neighbor is outside the grid or masked, 
    // check if this face should be a wall (no socket connection needed)
    bool isCornerBlock = parameters.generationSettings.cornerBlockIds.count(blockId) > 0;
    bool neighborOutsideOrMasked = !isValidGridPosition(neighborX, neighborY, neighborZ) || 
                                   (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(neighborX, neighborY, neighborZ));
    
    if (isCornerBlock && neighborOutsideOrMasked) {
        // For corner blocks, outward-facing sides should be walls, so no socket connection is expected
        // This is valid - the corner block's wall face is connecting to empty space
        return true;
    }
    
    if (neighborCell.collapsed && !neighborCell.blockTypeIds.empty()) {
        int neighborBlockId = neighborCell.blockTypeIds[0];
        int neighborRotation = neighborCell.blockRotations.empty() ? 0 : neighborCell.blockRotations[0];

        auto key = std::make_tuple(blockId, rotation, faceIndex);
        auto it = adjacencyTable.find(key);
        if (it != adjacencyTable.end())
            return it->second.count({neighborBlockId, neighborRotation}) > 0;
        return false;
    }
    if (!neighborCell.collapsed) {
        return true;
    }
    return true;
}

void BlockGenerator::propagateConstraints(int x, int y, int z) {
    std::queue<GridPosition> toPropagate;
    std::set<GridPosition> inQueue;
    toPropagate.push({x, y, z});
    inQueue.insert({x, y, z});

    while (!toPropagate.empty()) {
        GridPosition pos = toPropagate.front();
        toPropagate.pop();
        inQueue.erase(pos);

        const std::vector<std::tuple<int, int, int>> neighbors = {
            {pos.x, pos.y, pos.z+1}, {pos.x, pos.y, pos.z-1},
            {pos.x+1, pos.y, pos.z}, {pos.x-1, pos.y, pos.z},
            {pos.x, pos.y+1, pos.z}, {pos.x, pos.y-1, pos.z}
        };

        for (const auto& [nx, ny, nz] : neighbors) {
            if (!isValidGridPosition(nx, ny, nz)) continue;
            auto& neighbor = grid[nx][ny][nz];
            if (neighbor.collapsed) continue;

            // Save old possibilities for comparison
            auto oldPairs = neighbor.possibleBlockRotationPairs;
            updateCellPossibilities(nx, ny, nz);

            // If possibilities changed, propagate further
            if (neighbor.possibleBlockRotationPairs != oldPairs) {
                GridPosition npos{nx, ny, nz};
                if (inQueue.find(npos) == inQueue.end()) {
                    toPropagate.push(npos);
                    inQueue.insert(npos);
                }
            }
        }
    }
}

BlockMesh* BlockGenerator::generateMeshFromGrid() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;
    std::set<std::shared_ptr<Texture>> uniqueTextures;
    BlockMesh* blockMesh = new BlockMesh(vertices, indices, parameters, textures);
    unsigned int numThreads = std::thread::hardware_concurrency();
    std::vector<std::vector<std::tuple<int, glm::vec3, int>>> threadBlocks(numThreads);
    std::vector<std::set<std::shared_ptr<Texture>>> threadTextures(numThreads);

    auto meshWorker = [&](unsigned int threadId, unsigned int startX, unsigned int endX) {
        for (unsigned int x = startX; x < endX; x++) {
            for (unsigned int y = 0; y < parameters.gridHeight; y++)
                for (unsigned int z = 0; z < parameters.gridLength; z++) {
                    const auto& cell = grid[x][y][z];
                    if (!cell.collapsed || cell.blockTypeIds.empty()) continue;
                    for (size_t i = 0; i < cell.blockTypeIds.size(); i++) {
                        if (i >= cell.blockPositions.size()) continue;
                        int rotation = (i < cell.blockRotations.size()) ? cell.blockRotations[i] : 0;
                        threadBlocks[threadId].emplace_back(cell.blockTypeIds[i], cell.blockPositions[i], rotation);
                        if (controller && controller->GetBlockUI()) {
                            auto assets = controller->GetBlockUI()->GetLoadedAssets();
                            for (const auto& asset : assets) {
                                if (asset.id == cell.blockTypeIds[i] && asset.model) {
                                    for (auto* mesh : asset.model->getMeshes())
                                        if (mesh)
                                            for (auto& texture : mesh->textures)
                                                threadTextures[threadId].insert(texture);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
    };

    unsigned int chunkSize = parameters.gridWidth / numThreads, remainder = parameters.gridWidth % numThreads, startX = 0;
    std::vector<std::thread> workers;
    for (unsigned int t = 0; t < numThreads; ++t) {
        unsigned int endX = startX + chunkSize + (t < remainder ? 1 : 0);
        workers.emplace_back(meshWorker, t, startX, endX);
        startX = endX;
    }
    for (auto& th : workers) th.join();

    for (unsigned int t = 0; t < numThreads; ++t) {
        for (const auto& block : threadBlocks[t]) {
            int blockId; glm::vec3 pos; int rot;
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
    if (blockId < 0) return;
    Transform blockTransform;
    blockTransform.setPosition(position);
    blockTransform.setScale(parameters.blockScale);
    blockTransform.setRotation(glm::vec3(0.0f, static_cast<float>(rotation), 0.0f));
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
        if (mesh)
            for (auto& texture : mesh->textures)
                uniqueTextures.insert(texture);
    }
}

bool BlockGenerator::isValidGridPosition(int x, int y, int z) const {
    return x >= 0 && x < (int)parameters.gridWidth &&
           y >= 0 && y < (int)parameters.gridHeight &&
           z >= 0 && z < (int)parameters.gridLength;
}

glm::vec3 BlockGenerator::calculateBlockPosition(int x, int y, int z) const {
    float cellCenterX = (x * parameters.cellWidth) - parameters.halfWorldWidth;
    float cellCenterY = y * parameters.cellHeight;
    float cellCenterZ = (z * parameters.cellLength) - parameters.halfWorldLength;
    glm::vec3 pos = glm::vec3(cellCenterX, cellCenterY, cellCenterZ);
    return pos * parameters.gridScale;
}

BlockMesh* BlockGenerator::createEmptyMesh() {
    return new BlockMesh(std::vector<Vertex>(), std::vector<unsigned int>(), parameters);
}

void BlockGenerator::initializeBlockWeights() {
    auto& settings = parameters.generationSettings;
    settings.currentBlockCounts.clear();
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets) {
            if (settings.blockWeights.find(asset.id) == settings.blockWeights.end())
                settings.blockWeights[asset.id] = settings.defaultWeight;
            if (settings.maxBlockCounts.find(asset.id) == settings.maxBlockCounts.end())
                settings.maxBlockCounts[asset.id] = -1;
            settings.currentBlockCounts[asset.id] = 0;
        }
    }
}

void BlockGenerator::resetBlockCounts() {
    auto& settings = parameters.generationSettings;
    for (auto& [blockId, count] : settings.currentBlockCounts) count = 0;
}

bool BlockGenerator::canPlaceBlock(int blockId) const {
    auto& settings = parameters.generationSettings;
    auto maxIt = settings.maxBlockCounts.find(blockId);
    auto currentIt = settings.currentBlockCounts.find(blockId);
    int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
    bool isUnlimited = (maxIt == settings.maxBlockCounts.end() || maxIt->second == -1);
    return isUnlimited ? true : (currentCount < maxIt->second);
}

std::vector<int> BlockGenerator::getAvailableBlocks() const {
    std::vector<int> available;
    if (controller && controller->GetBlockUI()) {
        auto assets = controller->GetBlockUI()->GetLoadedAssets();
        for (const auto& asset : assets)
            if (canPlaceBlock(asset.id)) available.push_back(asset.id);
    }
    return available;
}

bool BlockGenerator::hasReachedLimit(int blockId) const {
    auto& settings = parameters.generationSettings;
    auto maxIt = settings.maxBlockCounts.find(blockId);
    auto currentIt = settings.currentBlockCounts.find(blockId);
    if (maxIt == settings.maxBlockCounts.end() || maxIt->second == -1) return false;
    int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
    return currentCount >= maxIt->second;
}

void BlockGenerator::incrementBlockCount(int blockId) {
    auto& settings = parameters.generationSettings;
    settings.currentBlockCounts[blockId]++;
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
    static const std::vector<int> opposites = {1, 0, 3, 2, 5, 4};
    return (faceIndex >= 0 && faceIndex < 6) ? opposites[faceIndex] : 0;
}

void BlockGenerator::generateRectangularCastle(std::mt19937& rng) {
    // Initialize the grid mask for rectangular castle generation
    initializeGridMask();
    
    // Find the first corner position to start generation
    GridPosition cornerStart = findFirstCornerPosition();
    
    // Try to place a corner block at the starting position
    if (!tryPlaceCornerBlock(cornerStart.x, cornerStart.y, cornerStart.z, rng)) {
        std::cerr << "Failed to place initial corner block at (" << cornerStart.x << ", " << cornerStart.y << ", " << cornerStart.z << ")" << std::endl;
        return;
    }
    
    // Use frontier-based WFC for the rest of the generation, but respect the grid mask
    std::priority_queue<FrontierCell, std::vector<FrontierCell>, std::greater<>> frontier;
    std::set<GridPosition> inFrontier;
    
    // Add neighbors of the initial corner to the frontier
    const std::vector<std::tuple<int, int, int>> neighborOffsets = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    
    for (const auto& [dx, dy, dz] : neighborOffsets) {
        int nx = cornerStart.x + dx, ny = cornerStart.y + dy, nz = cornerStart.z + dz;
        if (isValidGridPosition(nx, ny, nz) && !isGridCellMasked(nx, ny, nz) && !grid[nx][ny][nz].collapsed) {
            double entropy = calculateCellEntropy(grid[nx][ny][nz]);
            frontier.push({entropy, {nx, ny, nz}});
            inFrontier.insert({nx, ny, nz});
        }
    }
    
    // Main generation loop
    while (!frontier.empty()) {
        FrontierCell fc = frontier.top();
        frontier.pop();
        
        // Skip if cell is masked or already collapsed
        if (isGridCellMasked(fc.pos.x, fc.pos.y, fc.pos.z) || grid[fc.pos.x][fc.pos.y][fc.pos.z].collapsed) {
            inFrontier.erase(fc.pos);
            continue;
        }
        
        auto& cell = grid[fc.pos.x][fc.pos.y][fc.pos.z];
        if (cell.possibleBlockRotationPairs.empty()) {
            inFrontier.erase(fc.pos);
            continue;
        }
        
        // Collapse the cell (use corner block if it's a corner position)
        bool success = false;
        if (isCornerPosition(fc.pos.x, fc.pos.y, fc.pos.z)) {
            success = tryPlaceCornerBlock(fc.pos.x, fc.pos.y, fc.pos.z, rng);
        } else {
            success = collapseCell(fc.pos.x, fc.pos.y, fc.pos.z, rng);
        }
        
        if (!success) {
            inFrontier.erase(fc.pos);
            continue;
        }
        
        inFrontier.erase(fc.pos);
        
        // Add neighbors to frontier
        for (const auto& [dx, dy, dz] : neighborOffsets) {
            int nx = fc.pos.x + dx, ny = fc.pos.y + dy, nz = fc.pos.z + dz;
            GridPosition npos{nx, ny, nz};
            
            if (isValidGridPosition(nx, ny, nz) && !isGridCellMasked(nx, ny, nz) && 
                !grid[nx][ny][nz].collapsed && inFrontier.find(npos) == inFrontier.end()) {
                double entropy = calculateCellEntropy(grid[nx][ny][nz]);
                frontier.push({entropy, {nx, ny, nz}});
                inFrontier.insert({nx, ny, nz});
            }
        }
    }
}

void BlockGenerator::initializeGridMask() {
    gridMask.assign(parameters.gridWidth, 
        std::vector<std::vector<bool>>(parameters.gridHeight, 
            std::vector<bool>(parameters.gridLength, false)));
    
    // Create a hollow rectangle pattern
    // Only the perimeter of the rectangle should be unmasked (allowed for generation)
    for (unsigned int y = 0; y < parameters.gridHeight; ++y) {
        for (unsigned int x = 0; x < parameters.gridWidth; ++x) {
            for (unsigned int z = 0; z < parameters.gridLength; ++z) {
                // Check if this position is on the perimeter of the rectangle
                bool isPerimeter = (x == 0 || x == parameters.gridWidth - 1) || 
                                  (z == 0 || z == parameters.gridLength - 1);
                
                // Unmask (allow generation) only on the perimeter
                gridMask[x][y][z] = !isPerimeter;
            }
        }
    }
}

bool BlockGenerator::isGridCellMasked(int x, int y, int z) const {
    if (!isValidGridPosition(x, y, z)) return true;
    return gridMask[x][y][z];
}

bool BlockGenerator::isCornerPosition(int x, int y, int z) const {
    if (!isValidGridPosition(x, y, z)) return false;
    
    // A corner position is at the intersection of two perimeter edges
    bool isXEdge = (x == 0 || x == (int)parameters.gridWidth - 1);
    bool isZEdge = (z == 0 || z == (int)parameters.gridLength - 1);
    
    return isXEdge && isZEdge;
}

GridPosition BlockGenerator::findFirstCornerPosition() const {
    // Find the first corner position that is not masked
    for (int y = 0; y < (int)parameters.gridHeight; ++y) {
        for (int x = 0; x < (int)parameters.gridWidth; ++x) {
            for (int z = 0; z < (int)parameters.gridLength; ++z) {
                if (isCornerPosition(x, y, z) && !isGridCellMasked(x, y, z)) {
                    return {x, y, z};
                }
            }
        }
    }
    // Fallback to (0,0,0) if no corner found
    return {0, 0, 0};
}

bool BlockGenerator::tryPlaceCornerBlock(int x, int y, int z, std::mt19937& rng) {
    if (!isValidGridPosition(x, y, z)) return false;
    
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return false;
    
    // Get available corner blocks
    std::vector<int> cornerBlocks = getCornerBlocks();
    if (cornerBlocks.empty()) {
        std::cerr << "No corner blocks available for placement" << std::endl;
        return false;
    }
    
    // Try each corner block
    for (int cornerBlockId : cornerBlocks) {
        if (!canPlaceBlock(cornerBlockId)) continue;
        
        // Try to find the best rotation for this corner block
        int bestRotation = findBestCornerRotation(x, y, z, cornerBlockId);
        if (bestRotation != -1) {
            // Place the corner block with the best rotation
            cell.collapsed = true;
            cell.possibleBlockRotationPairs = {{cornerBlockId, bestRotation}};
            cell.blockTypeIds = {cornerBlockId};
            cell.blockRotations = {bestRotation};
            glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
            cell.blockPositions = {blockPosition};
            incrementBlockCount(cornerBlockId);
            
            // Reset the first block flag after placing the initial corner
            if (isFirstBlock) {
                isFirstBlock = false;
                std::cout << "First corner block placed successfully! Subsequent corners will require only 1 propagation direction." << std::endl;
            }
            
            // Propagate constraints
            propagateConstraints(x, y, z);
            return true;
        }
    }
    
    std::cerr << "Failed to place any corner block at (" << x << ", " << y << ", " << z << ")" << std::endl;
    return false;
}

int BlockGenerator::findBestCornerRotation(int x, int y, int z, int cornerBlockId) const {
    // Get the allowed rotations for this specific corner block from the socket system
    auto& templates = parameters.socketSystem.GetBlockTemplates();
    auto templateIt = templates.find(cornerBlockId);
    
    if (templateIt == templates.end()) {
        std::cerr << "Corner block template not found for ID: " << cornerBlockId << std::endl;
        return -1;
    }
    
    const auto& blockTemplate = templateIt->second;
    
    // Determine the corner type based on position in the rectangle
    int targetRotation = getCornerRotationForPosition(x, z);
    
    std::cout << "Trying corner block " << cornerBlockId << " at (" << x << "," << y << "," << z << ") with target rotation: " << targetRotation << std::endl;
    
    // Check if this is the first block (starting corner)
    bool isStartingCorner = isFirstBlock;
    
    // First, try the calculated target rotation if it's allowed
    for (int rotation : blockTemplate.allowedRotations) {
        if (rotation == targetRotation && isCornerRotationValid(x, y, z, cornerBlockId, rotation, isStartingCorner)) {
            std::cout << "Found valid target rotation: " << rotation << std::endl;
            return rotation;
        }
    }
    
    // If target rotation doesn't work, try rotations closest to the target
    std::vector<int> sortedRotations = blockTemplate.allowedRotations;
    std::sort(sortedRotations.begin(), sortedRotations.end(), [targetRotation](int a, int b) {
        int diffA = std::abs(a - targetRotation);
        int diffB = std::abs(b - targetRotation);
        // Handle wraparound for 270 vs 0 degree difference
        diffA = std::min(diffA, std::abs(diffA - 360));
        diffB = std::min(diffB, std::abs(diffB - 360));
        return diffA < diffB;
    });
    
    // Try each rotation in order of preference
    for (int rotation : sortedRotations) {
        std::cout << "Trying rotation: " << rotation << std::endl;
        if (isCornerRotationValid(x, y, z, cornerBlockId, rotation, isStartingCorner)) {
            std::cout << "Found valid fallback rotation: " << rotation << std::endl;
            return rotation;
        }
    }
    
    std::cout << "No valid rotation found for corner block " << cornerBlockId << " at (" << x << "," << y << "," << z << ")" << std::endl;
    return -1; // No valid rotation found
}

bool BlockGenerator::isCornerRotationValid(int x, int y, int z, int cornerBlockId, int rotation, bool isStartingCorner) const {
    // First check basic socket compatibility
    if (!isBlockValidAtPosition(x, y, z, cornerBlockId, rotation)) {
        std::cout << "Basic socket validation failed for corner block " << cornerBlockId << " at rotation " << rotation << std::endl;
        return false;
    }
    
    // Debug: Print socket information for the corner block with the specific rotation
    auto& templates = parameters.socketSystem.GetBlockTemplates();
    
    // First print the base template sockets
    auto templateIt = templates.find(cornerBlockId);
    if (templateIt != templates.end()) {
        const auto& blockTemplate = templateIt->second;
        std::cout << "Base corner block " << cornerBlockId << " socket types: ";
        for (int i = 0; i < 6; i++) {
            std::cout << "[" << i << "]=" << static_cast<int>(blockTemplate.sockets[i].type) << " ";
        }
        std::cout << std::endl;
    }
    
    // Then get and print the rotated sockets
    try {
        auto rotatedSockets = parameters.socketSystem.GetRotatedSockets(cornerBlockId, rotation);
        std::cout << "Corner block " << cornerBlockId << " at rotation " << rotation << " rotated socket types: ";
        for (int i = 0; i < 6; i++) {
            std::cout << "[" << i << "]=" << static_cast<int>(rotatedSockets[i].type) << " ";
        }
        std::cout << std::endl;
        
        // Manual rotation check - let's see if we can manually rotate the base sockets
        if (templateIt != templates.end()) {
            const auto& blockTemplate = templateIt->second;
            std::cout << "Manual rotation check for " << rotation << " degrees:" << std::endl;
            
            // For Y-axis rotation, face mapping should be:
            // 0 degrees: [+X, -X, +Y, -Y, +Z, -Z] = [0, 1, 2, 3, 4, 5]
            // 90 degrees: +X->+Z, -X->-Z, +Z->-X, -Z->+X
            // 180 degrees: +X->-X, -X->+X, +Z->-Z, -Z->+Z  
            // 270 degrees: +X->-Z, -X->+Z, +Z->+X, -Z->-X
            
            std::array<int, 6> faceMapping;
            if (rotation == 0) {
                faceMapping = {0, 1, 2, 3, 4, 5}; // No rotation
            } else if (rotation == 90) {
                faceMapping = {4, 5, 2, 3, 1, 0}; // +X->+Z, -X->-Z, +Z->-X, -Z->+X
            } else if (rotation == 180) {
                faceMapping = {1, 0, 2, 3, 5, 4}; // +X->-X, -X->+X, +Z->-Z, -Z->+Z
            } else if (rotation == 270) {
                faceMapping = {5, 4, 2, 3, 0, 1}; // +X->-Z, -X->+Z, +Z->+X, -Z->-X
            } else {
                faceMapping = {0, 1, 2, 3, 4, 5}; // Default
            }
            
            std::cout << "Manual rotated sockets: ";
            for (int i = 0; i < 6; i++) {
                int originalFace = faceMapping[i];
                std::cout << "[" << i << "]=" << static_cast<int>(blockTemplate.sockets[originalFace].type) << " ";
            }
            std::cout << std::endl;
        }
        
    } catch (...) {
        std::cout << "Failed to get rotated sockets for block " << cornerBlockId << " at rotation " << rotation << std::endl;
    }
    
    // For corners, we need to check propagation directions in the XZ plane
    // Try different face mapping order: +Z, -Z, +X, -X instead of +X, -X, +Z, -Z
    const std::vector<std::tuple<int, int, int, int>> horizontalNeighbors = {
        {x, y, z+1, 4}, {x, y, z-1, 5}, {x+1, y, z, 0}, {x-1, y, z, 1}
    };
    
    int validPropagationDirections = 0;
    
    for (const auto& [nx, ny, nz, faceIndex] : horizontalNeighbors) {
        // Check if this direction can be used for propagation
        if (isValidGridPosition(nx, ny, nz) && !isGridCellMasked(nx, ny, nz)) {
            // This is a valid direction for propagation
            validPropagationDirections++;
            std::cout << "Valid propagation direction: face " << faceIndex << " to (" << nx << "," << ny << "," << nz << ")" << std::endl;
        } else {
            std::cout << "Invalid propagation direction: face " << faceIndex << " to (" << nx << "," << ny << "," << nz << ") - ";
            if (!isValidGridPosition(nx, ny, nz)) {
                std::cout << "outside grid";
            } else if (isGridCellMasked(nx, ny, nz)) {
                std::cout << "masked";
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "Corner at (" << x << "," << y << "," << z << ") has " << validPropagationDirections 
              << " valid propagation directions, isStarting: " << isStartingCorner << std::endl;
    
    if (isStartingCorner) {
        // Starting corner needs at least 2 propagation directions
        return validPropagationDirections >= 2;
    } else {
        // Non-starting corners need at least 1 propagation direction
        return validPropagationDirections >= 1;
    }
}

int BlockGenerator::getCornerRotationForPosition(int x, int z) const {
    // Determine which corner this is and return the appropriate rotation
    bool isLeftEdge = (x == 0);
    bool isRightEdge = (x == (int)parameters.gridWidth - 1);
    bool isFrontEdge = (z == 0);
    bool isBackEdge = (z == (int)parameters.gridLength - 1);
    
    // For corners, we need to think about which faces should be walls vs connectable
    // A corner piece should have walls facing outward (toward the void) and connectable faces inward
    // Rotate all corners by another 90 degrees clockwise
    
    if (isLeftEdge && isFrontEdge) {
        // Bottom-left corner (0,0): try 90 degrees (was 180)
        return 90;
    }
    else if (isRightEdge && isFrontEdge) {
        // Bottom-right corner (max,0): try 0 degrees (was 270)
        return 0;
    }
    else if (isRightEdge && isBackEdge) {
        // Top-right corner (max,max): try 90 degrees (was 0)
        return 90;
    }
    else if (isLeftEdge && isBackEdge) {
        // Top-left corner (0,max): try 180 degrees (was 90)
        return 180;
    }
    
    // Default fallback
    return 0;
}

std::vector<int> BlockGenerator::getCornerBlocks() const {
    std::vector<int> cornerBlocks;
    
    // Convert set to vector for easier handling
    for (int blockId : parameters.generationSettings.cornerBlockIds) {
        cornerBlocks.push_back(blockId);
    }
    
    return cornerBlocks;
}
