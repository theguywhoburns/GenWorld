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
#include <map>
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
    
    // Check if minimum requirements are met
    if (!hasMetMinimumRequirements()) {
        auto blocksNeeded = getBlocksNeedingMinCount();

        for (int blockId : blocksNeeded) {
            auto& settings = parameters.generationSettings;
            auto currentIt = settings.currentBlockCounts.find(blockId);
            auto minIt = settings.minBlockCounts.find(blockId);
            int current = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
            int needed = (minIt != settings.minBlockCounts.end()) ? minIt->second : 0;
        }
        std::cout << std::endl;
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
    runSingleWFCAttempt(rng);
}


double BlockGenerator::calculateCellEntropy(const GridCell& cell) const {
    if (cell.collapsed || cell.possibleBlockRotationPairs.empty()) return 0.0;
    double sum = 0.0, logSum = 0.0;
    auto& weights = parameters.generationSettings.blockWeights;
    auto blocksNeedingMin = getBlocksNeedingMinCount();
    
    for (const auto& pair : cell.possibleBlockRotationPairs) {
        int id = pair.first;
        double w = weights.count(id) ? weights.at(id) : 1.0;
        
        if (std::find(blocksNeedingMin.begin(), blocksNeedingMin.end(), id) != blocksNeedingMin.end()) {
            w *= 1.3; // Smaller boost (30% instead of 100%) for better distribution
        }
        
        sum += w;
        logSum += w * std::log(w);
    }
    return std::log(sum) - (logSum / sum);
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
    
    // Filter out blocks with zero weight first
    std::vector<int> validBlocks;
    float totalWeight = 0.0f;
    
    for (int blockId : blocks) {
        auto it = settings.blockWeights.find(blockId);
        float weight = (it != settings.blockWeights.end()) ? it->second : 1.0f;
        
        // Only include blocks with positive weight
        if (weight > 0.0f) {
            validBlocks.push_back(blockId);
            totalWeight += weight;
        }
    }
    
    // If no blocks have positive weight, fall back to uniform selection from all blocks
    if (validBlocks.empty() || totalWeight <= 0.0f) {
        std::uniform_int_distribution<int> dist(0, blocks.size() - 1);
        return blocks[dist(rng)];
    }
    
    // Weighted selection from blocks with positive weights
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float randomValue = dist(rng), currentWeight = 0.0f;
    
    for (int blockId : validBlocks) {
        auto it = settings.blockWeights.find(blockId);
        float weight = (it != settings.blockWeights.end()) ? it->second : 1.0f;
        currentWeight += weight;
        if (randomValue <= currentWeight) return blockId;
    }
    
    return validBlocks.back();
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
    
    // Skip corner blocks - they use special placement logic
    if (settings.cornerBlockIds.count(blockId) > 0) {
        return true;
    }
    
    auto maxIt = settings.maxBlockCounts.find(blockId);
    auto currentIt = settings.currentBlockCounts.find(blockId);
    int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
    bool isUnlimited = (maxIt == settings.maxBlockCounts.end() || maxIt->second == -1);
    
    // If unlimited, no max constraint, otherwise check max limit
    if (!isUnlimited && currentCount >= maxIt->second) {
        return false;
    }
    
    return true;
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

bool BlockGenerator::hasMetMinimumRequirements() const {
    auto& settings = parameters.generationSettings;
    
    for (const auto& [blockId, minCount] : settings.minBlockCounts) {
        // Skip corner blocks - they use special placement logic
        if (settings.cornerBlockIds.count(blockId) > 0) {
            continue;
        }
        
        auto currentIt = settings.currentBlockCounts.find(blockId);
        int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
        
        if (currentCount < minCount) {
            return false;
        }
    }
    
    return true;
}

std::vector<int> BlockGenerator::getBlocksNeedingMinCount() const {
    auto& settings = parameters.generationSettings;
    std::vector<int> blocksNeeded;
    
    for (const auto& [blockId, minCount] : settings.minBlockCounts) {
        // Skip corner blocks - they use special placement logic
        if (settings.cornerBlockIds.count(blockId) > 0) {
            continue;
        }
        
        auto currentIt = settings.currentBlockCounts.find(blockId);
        int currentCount = (currentIt != settings.currentBlockCounts.end()) ? currentIt->second : 0;
        
        if (currentCount < minCount && canPlaceBlock(blockId)) {
            blocksNeeded.push_back(blockId);
        }
    }
    
    return blocksNeeded;
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
            success = collapseCellWFC(fc.pos.x, fc.pos.y, fc.pos.z, rng);
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
    
    std::cout << "Available corner blocks: ";
    for (int blockId : cornerBlocks) {
        std::cout << blockId << " ";
    }
    std::cout << std::endl;
    
    // Shuffle the corner blocks for random selection
    std::shuffle(cornerBlocks.begin(), cornerBlocks.end(), rng);
    
    // Try each corner block in random order
    for (int cornerBlockId : cornerBlocks) {
        std::cout << "Trying corner block " << cornerBlockId << " at (" << x << "," << y << "," << z << ")" << std::endl;
        
        if (!canPlaceBlock(cornerBlockId)) {
            std::cout << "Corner block " << cornerBlockId << " cannot be placed (limit reached)" << std::endl;
            continue;
        }
        
        // Try to find the best rotation for this corner block
        int bestRotation = findBestCornerRotation(x, y, z, cornerBlockId);
        if (bestRotation != -1) {
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
            propagateWave({x, y, z});
            return true;
        } else {
            std::cout << "Corner block " << cornerBlockId << " failed rotation validation" << std::endl;
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
    
    // Detect the preset of this corner block (which two faces are exterior in base orientation)
    int preset = detectCornerBlockPreset(cornerBlockId);
    if (preset == -1) {
        std::cerr << "Could not detect preset for corner block " << cornerBlockId << std::endl;
        return -1;
    }
    
    // Determine which corner position this is
    int cornerPosition = getCornerPosition(x, z);
    if (cornerPosition == -1) {
        std::cerr << "Invalid corner position (" << x << "," << z << ")" << std::endl;
        return -1;
    }
    
    // Get the correct rotation for this preset at this corner position
    int targetRotation = getRotationForPresetAtCorner(preset, cornerPosition);
    
    bool isStartingCorner = isFirstBlock;

    for (int rotation : blockTemplate.allowedRotations) {
        if (rotation == targetRotation && isCornerRotationValid(x, y, z, cornerBlockId, rotation, isStartingCorner)) {
            return rotation;
        }
    }
    
    // If target rotation doesn't work, try all other allowed rotations
    for (int rotation : blockTemplate.allowedRotations) {
        if (rotation != targetRotation) {
            if (isCornerRotationValid(x, y, z, cornerBlockId, rotation, isStartingCorner)) {
                return rotation;
            }
        }
    }
    
    return -1;
}

bool BlockGenerator::isCornerRotationValid(int x, int y, int z, int cornerBlockId, int rotation, bool isStartingCorner) const {
    // First check basic socket compatibility
    if (!isBlockValidAtPosition(x, y, z, cornerBlockId, rotation)) {
        return false;
    }

    auto& templates = parameters.socketSystem.GetBlockTemplates();

    auto templateIt = templates.find(cornerBlockId);

    try {
        auto rotatedSockets = parameters.socketSystem.GetRotatedSockets(cornerBlockId, rotation);
        
        // Manual rotation check - let's see if we can manually rotate the base sockets
        if (templateIt != templates.end()) {
            const auto& blockTemplate = templateIt->second;
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
            
        }
        
    } catch (...) {
        std::cout << "Failed to get rotated sockets for block " << cornerBlockId << " at rotation " << rotation << std::endl;
    }
    
    const std::vector<std::tuple<int, int, int, int>> horizontalNeighbors = {
        {x, y, z+1, 4}, {x, y, z-1, 5}, {x+1, y, z, 0}, {x-1, y, z, 1}
    };
    
    int validPropagationDirections = 0;
    
    for (const auto& [nx, ny, nz, faceIndex] : horizontalNeighbors) {
        // Check if this direction can be used for propagation
        if (isValidGridPosition(nx, ny, nz) && !isGridCellMasked(nx, ny, nz)) {
            // This is a valid direction for propagation
            validPropagationDirections++;
        }
    }
    if (isStartingCorner) {
        return validPropagationDirections >= 2;
    } else {
        return validPropagationDirections >= 1;
    }
}

int BlockGenerator::getCornerRotationForPosition(int x, int z) const {
    bool isLeftEdge = (x == 0);
    bool isRightEdge = (x == (int)parameters.gridWidth - 1);
    bool isFrontEdge = (z == 0);
    bool isBackEdge = (z == (int)parameters.gridLength - 1);
    
    if (isLeftEdge && isFrontEdge) {
        return 90;
    }
    else if (isRightEdge && isFrontEdge) {
        return 0;
    }
    else if (isRightEdge && isBackEdge) {
        return 90;
    }
    else if (isLeftEdge && isBackEdge) {
        return 180;
    }
    
    return 0;
}

int BlockGenerator::detectCornerBlockPreset(int cornerBlockId) const {
    
    auto& templates = parameters.socketSystem.GetBlockTemplates();
    auto templateIt = templates.find(cornerBlockId);
    
    if (templateIt == templates.end()) {
        std::cerr << "Block template not found for ID: " << cornerBlockId << std::endl;
        return -1;
    }
    
    const auto& blockTemplate = templateIt->second;
    
    bool posXIsWall = (blockTemplate.sockets[0].type == SocketType::WALL);
    bool negXIsWall = (blockTemplate.sockets[1].type == SocketType::WALL);
    bool posZIsWall = (blockTemplate.sockets[4].type == SocketType::WALL);
    bool negZIsWall = (blockTemplate.sockets[5].type == SocketType::WALL);
    
    if (posXIsWall && posZIsWall) {
        return 0;
    }
    else if (posXIsWall && negZIsWall) {
        return 1;
    }
    else if (negXIsWall && posZIsWall) {
        return 2;
    }
    else if (negXIsWall && negZIsWall) {
        return 3;
    }
    else {
        std::cerr << "Could not determine preset for block " << cornerBlockId 
                  << " - invalid wall configuration for a corner block" << std::endl;
        return -1;
    }
}

int BlockGenerator::getCornerPosition(int x, int z) const {
    bool isLeftEdge = (x == 0);
    bool isRightEdge = (x == (int)parameters.gridWidth - 1);
    bool isFrontEdge = (z == 0);
    bool isBackEdge = (z == (int)parameters.gridLength - 1);
    
    if (isLeftEdge && isFrontEdge) {
        return 0; // bottom-left
    }
    else if (isRightEdge && isFrontEdge) {
        return 1; // bottom-right
    }
    else if (isRightEdge && isBackEdge) {
        return 2; // top-right
    }
    else if (isLeftEdge && isBackEdge) {
        return 3; // top-left
    }
    
    return -1; // Not a corner
}

int BlockGenerator::getRotationForPresetAtCorner(int preset, int cornerPosition) const {
    static const int rotationTable[4][4] = {
        // Preset 0 (+X,+Z exterior in base): needs rotation to place exterior at required directions
        {180, 90, 0, 270},   // For positions 0,1,2,3

        // Preset 1 (+X,-Z exterior in base):
        {90, 0, 270, 180},   // For positions 0,1,2,3
        
        // Preset 2 (-X,+Z exterior in base):  
        {0, 90, 180, 270},    // For positions 0,1,2,3
        
        // Preset 3 (-X,-Z exterior in base):
        {270, 180, 90, 0},   // For positions 0,1,2,3
    };
    
    if (preset < 0 || preset >= 4 || cornerPosition < 0 || cornerPosition >= 4) {
        std::cerr << "Invalid preset (" << preset << ") or corner position (" << cornerPosition << ")" << std::endl;
        return 0;
    }
    
    int rotation = rotationTable[preset][cornerPosition];
    
    return rotation;
}

std::vector<int> BlockGenerator::getCornerBlocks() const {
    std::vector<int> cornerBlocks;
    
    for (int blockId : parameters.generationSettings.cornerBlockIds) {
        cornerBlocks.push_back(blockId);
    }
    
    return cornerBlocks;
}

void BlockGenerator::incrementBlockCount(int blockId) {
    auto& settings = parameters.generationSettings;
    settings.currentBlockCounts[blockId]++;
}

bool BlockGenerator::shouldPrioritizeMinCountBlock(int x, int y, int z, int blockId) const {
    auto& settings = parameters.generationSettings;
    
    // Check if this block needs minimum count
    auto blocksNeeded = getBlocksNeedingMinCount();
    if (std::find(blocksNeeded.begin(), blocksNeeded.end(), blockId) == blocksNeeded.end()) {
        return false; // Block doesn't need more placement
    }
    
    // Check nearby cells to avoid clustering
    int nearbyCount = 0;
    int searchRadius = 3; // Check within 3-cell radius
    
    for (int dx = -searchRadius; dx <= searchRadius; dx++) {
        for (int dy = -searchRadius; dy <= searchRadius; dy++) {
            for (int dz = -searchRadius; dz <= searchRadius; dz++) {
                int nx = x + dx, ny = y + dy, nz = z + dz;
                if (isValidGridPosition(nx, ny, nz) && grid[nx][ny][nz].collapsed) {
                    for (int placedBlockId : grid[nx][ny][nz].blockTypeIds) {
                        if (placedBlockId == blockId) {
                            nearbyCount++;
                        }
                    }
                }
            }
        }
    }
    
    return nearbyCount < 2; // Allow max 2 of the same type in nearby area
}

bool BlockGenerator::collapseCellWFC(int x, int y, int z, std::mt19937& rng) {
    if (!isValidGridPosition(x, y, z)) return false;
    
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return true;
    
    validateCellPossibilitySpace(x, y, z);
    
    if (cell.possibleBlockRotationPairs.empty()) {
        return false;
    }
    
    // Use weighted selection for block choice
    std::vector<std::pair<int, int>> priorityPairs;
    auto blocksNeeded = getBlocksNeedingMinCount();
    
    // Check for blocks that need minimum count
    for (const auto& pair : cell.possibleBlockRotationPairs) {
        int blockId = pair.first;
        if (std::find(blocksNeeded.begin(), blocksNeeded.end(), blockId) != blocksNeeded.end()) {
            if (shouldPrioritizeMinCountBlock(x, y, z, blockId)) {
                priorityPairs.push_back(pair);
            }
        }
    }
    
    // Use priority pairs if available, otherwise use all possibilities
    auto& finalPairs = priorityPairs.empty() ? cell.possibleBlockRotationPairs : priorityPairs;
    
    // Select weighted pair
    std::vector<int> uniqueBlocks;
    std::map<int, std::vector<int>> blockToRotations;
    
    // Group pairs by block ID
    for (const auto& pair : finalPairs) {
        int blockId = pair.first;
        int rotation = pair.second;
        
        if (blockToRotations.find(blockId) == blockToRotations.end()) {
            uniqueBlocks.push_back(blockId);
            blockToRotations[blockId] = std::vector<int>();
        }
        blockToRotations[blockId].push_back(rotation);
    }
    
    // Select block using weights
    int chosenBlockId = selectWeightedBlock(uniqueBlocks, rng);
    
    // Select random rotation for the chosen block
    auto& availableRotations = blockToRotations[chosenBlockId];
    std::uniform_int_distribution<size_t> rotDist(0, availableRotations.size() - 1);
    int chosenRotation = availableRotations[rotDist(rng)];
    
    std::pair<int, int> chosenPair = {chosenBlockId, chosenRotation};
    
    // Collapse the cell
    cell.collapsed = true;
    cell.possibleBlockRotationPairs = {chosenPair};
    cell.blockTypeIds = {chosenPair.first};
    cell.blockRotations = {chosenPair.second};
    glm::vec3 blockPosition = calculateBlockPosition(x, y, z);
    cell.blockPositions = {blockPosition};
    incrementBlockCount(chosenPair.first);
    
    return true;
}

void BlockGenerator::propagateWave(const GridPosition& startPos) {
    std::vector<GridPosition> neighbors = getNeighborPositions(startPos);
    
    // Add neighbors to fringe
    for (const auto& neighbor : neighbors) {
        propagationFringe.push(neighbor);
    }
    
    // Propagate changes until fringe is empty
    while (!propagationFringe.empty()) {
        GridPosition currentPos = propagationFringe.top();
        propagationFringe.pop();
        
        if (!isValidGridPosition(currentPos.x, currentPos.y, currentPos.z)) continue;
        
        auto& cell = grid[currentPos.x][currentPos.y][currentPos.z];
        
        // Validate possibility space - remove invalid block/rotation pairs
        bool changed = validateCellPossibilitySpace(currentPos.x, currentPos.y, currentPos.z);
        
        // Remove from duplicate set since we processed it
        duplicateSet.erase(currentPos);
        
        // If cell has only one possibility and isn't collapsed, collapse it
        if (cell.possibleBlockRotationPairs.size() == 1 && !cell.collapsed) {
            auto chosenPair = cell.possibleBlockRotationPairs[0];
            cell.collapsed = true;
            cell.blockTypeIds = {chosenPair.first};
            cell.blockRotations = {chosenPair.second};
            glm::vec3 blockPosition = calculateBlockPosition(currentPos.x, currentPos.y, currentPos.z);
            cell.blockPositions = {blockPosition};
            incrementBlockCount(chosenPair.first);
        }
        
        if (changed) {
            std::vector<GridPosition> currentNeighbors = getNeighborPositions(currentPos);
            for (const auto& neighbor : currentNeighbors) {
                if (!isValidGridPosition(neighbor.x, neighbor.y, neighbor.z)) continue;
                
                auto& neighborCell = grid[neighbor.x][neighbor.y][neighbor.z];
                
                if (!neighborCell.collapsed && duplicateSet.find(neighbor) == duplicateSet.end()) {
                    propagationFringe.push(neighbor);
                    duplicateSet.insert(neighbor);
                }
            }
        }
    }
    
    // Clear duplicate set after wave propagation
    duplicateSet.clear();
}

std::vector<GridPosition> BlockGenerator::getNeighborPositions(const GridPosition& pos) const {
    std::vector<GridPosition> neighbors;
    
    const std::vector<std::tuple<int, int, int>> offsets = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };
    
    for (const auto& [dx, dy, dz] : offsets) {
        int nx = pos.x + dx;
        int ny = pos.y + dy;
        int nz = pos.z + dz;
        
        if (isValidGridPosition(nx, ny, nz)) {
            neighbors.push_back({nx, ny, nz});
        }
    }
    
    return neighbors;
}

bool BlockGenerator::validateCellPossibilitySpace(int x, int y, int z) {
    auto& cell = grid[x][y][z];
    if (cell.collapsed) return false;
    
    std::vector<std::pair<int, int>> validPairs;

    for (const auto& pair : cell.possibleBlockRotationPairs) {
        int blockId = pair.first;
        int rotation = pair.second;

        if (!canPlaceBlock(blockId)) continue;
        
        if (isBlockValidAtPosition(x, y, z, blockId, rotation)) {
            validPairs.push_back(pair);
        }
    }

    bool changed = (validPairs.size() != cell.possibleBlockRotationPairs.size());
    
    cell.possibleBlockRotationPairs = validPairs;
    
    return changed;
}

bool BlockGenerator::runSingleWFCAttempt(std::mt19937& rng) {
    // Initialize frontier using true bottom-up approach
    std::priority_queue<FrontierCell, std::vector<FrontierCell>, std::greater<>> frontier;
    std::set<GridPosition> inFrontier;
    
    // Clear propagation data structures
    while (!propagationFringe.empty()) propagationFringe.pop();
    duplicateSet.clear();
    
    // Start from the absolute bottom layer (y=0) and work upward
    int startY = 0;
    
    // Find the lowest Y layer that has valid, uncollapsed cells
    for (int y = 0; y < (int)parameters.gridHeight; ++y) {
        bool hasValidCells = false;
        for (int x = 0; x < (int)parameters.gridWidth; ++x) {
            for (int z = 0; z < (int)parameters.gridLength; ++z) {
                // Skip masked cells
                if (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(x, y, z)) {
                    continue;
                }
                
                auto& cell = grid[x][y][z];
                if (!cell.collapsed && !cell.possibleBlockRotationPairs.empty()) {
                    hasValidCells = true;
                    break;
                }
            }
            if (hasValidCells) break;
        }
        
        if (hasValidCells) {
            startY = y;
            break;
        }
    }
    
    for (int x = 0; x < (int)parameters.gridWidth; ++x) {
        for (int z = 0; z < (int)parameters.gridLength; ++z) {
            // Skip masked cells
            if (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(x, startY, z)) {
                continue;
            }
            
            auto& cell = grid[x][startY][z];
            if (!cell.collapsed && !cell.possibleBlockRotationPairs.empty()) {
                double entropy = calculateCellEntropy(cell);
                // Add Y-based priority: lower Y = higher priority (lower entropy value)
                entropy += (startY * 0.001); // Small bias toward lower layers
                
                frontier.push({entropy, {x, startY, z}});
                inFrontier.insert({x, startY, z});
            }
        }
    }
    
    const std::vector<std::tuple<int, int, int>> neighborOffsets = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    
    int iterationCount = 0;
    int consecutiveSkips = 0;
    const int maxConsecutiveSkips = 10;
    
    while (!frontier.empty()) {
        FrontierCell fc = frontier.top();
        frontier.pop();
        
        // Skip if already processed or no longer valid
        auto& cell = grid[fc.pos.x][fc.pos.y][fc.pos.z];
        if (cell.collapsed || cell.possibleBlockRotationPairs.empty()) {
            inFrontier.erase(fc.pos);
            consecutiveSkips++;
            
            if (consecutiveSkips > maxConsecutiveSkips && frontier.empty()) {
                std::cout << "Frontier expansion complete - no more connected cells to process" << std::endl;
                break; // Exit the loop instead of adding disconnected cells
            }
            continue;
        }
        
        consecutiveSkips = 0; 
        
        // Try to collapse the cell
        if (!collapseCellWFC(fc.pos.x, fc.pos.y, fc.pos.z, rng)) {
            
            // Mark the cell as permanently failed (no possibilities)
            cell.possibleBlockRotationPairs.clear();
            inFrontier.erase(fc.pos);
            
            // Continue with next frontier cell - never abort
            continue;
        }
        
        inFrontier.erase(fc.pos);
        
        // Propagate constraints from this cell
        propagateWave(fc.pos);
        
        for (const auto& [dx, dy, dz] : neighborOffsets) {
            int nx = fc.pos.x + dx, ny = fc.pos.y + dy, nz = fc.pos.z + dz;
            GridPosition npos{nx, ny, nz};
            
            if (isValidGridPosition(nx, ny, nz) && 
                !(parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(nx, ny, nz)) &&
                !grid[nx][ny][nz].collapsed && 
                !grid[nx][ny][nz].possibleBlockRotationPairs.empty() &&
                inFrontier.find(npos) == inFrontier.end()) {
                
                double entropy = calculateCellEntropy(grid[nx][ny][nz]);
                entropy += (ny * 0.1);
                
                frontier.push({entropy, npos});
                inFrontier.insert(npos);
            }
        }
    }

    return true;
}

bool BlockGenerator::isGenerationComplete() const {
    for (int x = 0; x < (int)parameters.gridWidth; ++x) {
        for (int y = 0; y < (int)parameters.gridHeight; ++y) {
            for (int z = 0; z < (int)parameters.gridLength; ++z) {
                // Skip masked cells
                if (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(x, y, z)) {
                    continue;
                }
                
                const auto& cell = grid[x][y][z];
                // Consider a cell "complete" if it's either collapsed OR has no possibilities (failed)
                if (!cell.collapsed && !cell.possibleBlockRotationPairs.empty()) {
                    return false; // Still has possibilities to be collapsed
                }
            }
        }
    }
    return true;
}

bool BlockGenerator::hasContradictions() const {
    for (int x = 0; x < (int)parameters.gridWidth; ++x) {
        for (int y = 0; y < (int)parameters.gridHeight; ++y) {
            for (int z = 0; z < (int)parameters.gridLength; ++z) {
                // Skip masked cells
                if (parameters.generationSettings.isGridMaskEnabled && isGridCellMasked(x, y, z)) {
                    continue;
                }
                
                const auto& cell = grid[x][y][z];
                // Check for cells with no possibilities (contradiction)
                if (!cell.collapsed && cell.possibleBlockRotationPairs.empty()) {
                    return true;
                }
            }
        }
    }
    return false;
}
