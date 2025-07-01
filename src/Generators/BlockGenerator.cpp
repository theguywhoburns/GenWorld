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

std::mutex gridMutex;

BlockGenerator::BlockGenerator(BlockController* controller) : controller(controller) { initializeDefaults(); }
BlockGenerator::BlockGenerator() : controller(nullptr) { initializeDefaults(); }
BlockGenerator::~BlockGenerator() { delete generatorMesh; }

void BlockGenerator::initializeDefaults() {
    parameters = {20, 10, 20, 5.0f, 5.0f, 5.0f, 1.0f};
    updateWorldDimensions();
    generatorMesh = nullptr;
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


    generateGridFrontierWFC(mainRng);

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
    // Start with the initial block at the center
    
    std::uniform_int_distribution<int> distX(0, parameters.gridWidth - 1);
    std::uniform_int_distribution<int> distY(0, parameters.gridHeight - 1);
    std::uniform_int_distribution<int> distZ(0, parameters.gridLength - 1);
    int centerX = distX(rng);
    int centerY = distY(rng);
    int centerZ = distZ(rng);

    if (!placeRandomBlockAt(centerX, centerY, centerZ, rng)) {
        std::cerr << "Failed to place initial block at center" << std::endl;
        return;
    }

    // Frontier: cells whose possibilities just changed and need to be collapsed
    std::queue<GridPosition> frontier;
    std::set<GridPosition> inFrontier;

    // Add all neighbors of the initial cell to the frontier
    const std::vector<std::tuple<int, int, int>> neighborOffsets = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    for (const auto& [dx, dy, dz] : neighborOffsets) {
        int nx = centerX + dx, ny = centerY + dy, nz = centerZ + dz;
        if (isValidGridPosition(nx, ny, nz) && !grid[nx][ny][nz].collapsed) {
            frontier.push({nx, ny, nz});
            inFrontier.insert({nx, ny, nz});
        }
    }

    // Main loop: process the frontier
    while (!frontier.empty()) {
        // Find the lowest-entropy cell in the frontier
        double minEntropy = std::numeric_limits<double>::max();
        GridPosition bestPos = {-1, -1, -1};
        size_t frontierSize = frontier.size();
        for (size_t i = 0; i < frontierSize; ++i) {
            GridPosition pos = frontier.front();
            frontier.pop();
            auto& cell = grid[pos.x][pos.y][pos.z];
            if (cell.collapsed || cell.possibleBlockRotationPairs.empty()) {
                inFrontier.erase(pos);
                continue;
            }
            double entropy = calculateCellEntropy(cell);
            if (entropy < minEntropy) {
                if (bestPos.x != -1) frontier.push(bestPos); // requeue previous best
                minEntropy = entropy;
                bestPos = pos;
            } else {
                frontier.push(pos);
            }
        }
        if (bestPos.x == -1) break; // No valid cells left

        // Collapse the best cell
        collapseCell(bestPos.x, bestPos.y, bestPos.z, rng);
        inFrontier.erase(bestPos);

        // Add its neighbors to the frontier if not already collapsed or in frontier
        for (const auto& [dx, dy, dz] : neighborOffsets) {
            int nx = bestPos.x + dx, ny = bestPos.y + dy, nz = bestPos.z + dz;
            GridPosition npos{nx, ny, nz};
            if (isValidGridPosition(nx, ny, nz) && !grid[nx][ny][nz].collapsed && inFrontier.find(npos) == inFrontier.end()) {
                frontier.push(npos);
                inFrontier.insert(npos);
            }
        }
    }
}

void BlockGenerator::generateGridMultithreaded(std::mt19937& mainRng) {
    const unsigned int numThreads = 1;
    std::vector<std::thread> workerThreads;
    std::atomic<bool> workCompleted(false);

    auto processChunk = [&](unsigned int) {
        std::mt19937 threadRng(mainRng());
        while (!workCompleted) {
            GridPosition nextPos{-1, -1, -1};
            {
                std::lock_guard<std::mutex> lock(gridMutex);
                nextPos = findLowestEntropyCell(threadRng);
                if (nextPos.x == -1) workCompleted = true;
            }
            if (nextPos.x == -1) break;
            {
                std::lock_guard<std::mutex> lock(gridMutex);
                if (!grid[nextPos.x][nextPos.y][nextPos.z].collapsed)
                    collapseCell(nextPos.x, nextPos.y, nextPos.z, threadRng);
            }
        }
    };

    for (unsigned int i = 0; i < numThreads; i++)
        workerThreads.emplace_back(processChunk, i);
    for (auto& thread : workerThreads) thread.join();
    processRemainingCells(mainRng);
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
GridPosition BlockGenerator::findLowestEntropyCell(std::mt19937& rng) {
    double minEntropy = std::numeric_limits<double>::max();
    std::vector<GridPosition> candidates;
    for (unsigned int x = 0; x < parameters.gridWidth; x++) {
        for (unsigned int y = 0; y < parameters.gridHeight; y++) {
            for (unsigned int z = 0; z < parameters.gridLength; z++) {
                const auto& cell = grid[x][y][z];
                if (cell.collapsed || cell.possibleBlockRotationPairs.empty()) continue;
                double entropy = calculateCellEntropy(cell) + (double)rng() / rng.max() * 1e-6; // Add noise to break ties
                if (entropy < minEntropy) {
                    minEntropy = entropy;
                    candidates.clear();
                    candidates.push_back({(int)x, (int)y, (int)z});
                } else if (entropy == minEntropy) {
                    candidates.push_back({(int)x, (int)y, (int)z});
                }
            }
        }
    }

    if (candidates.empty()) return {-1, -1, -1};
    std::uniform_int_distribution<int> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
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

    std::cout << "Collapsing cell at (" << x << ", " << y << ", " << z << ") to block type "
              << chosenBlockType << " with rotation " << chosenRotation << std::endl;

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
    std::vector<std::pair<int, int>> allPairs;
    for (const auto& [blockId, blockTemplate] : templates) {
        for (int rotation : blockTemplate.allowedRotations) {
            allPairs.emplace_back(blockId, rotation);
        }
    }
    for (auto& layer : grid) {
        for (auto& row : layer) {
            for (auto& cell : row) {
                cell = {allPairs, false, {}, {}, {}, 0.0f};
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
        for (int rotation : blockTemplate.allowedRotations) {
            if (isBlockValidAtPosition(x, y, z, blockId, rotation)) {
                validPairs.emplace_back(blockId, rotation);
            }
        }
    }
    cell.possibleBlockRotationPairs = validPairs;
    std::cout << "Updated cell at (" << x << ", " << y << ", " << z << ") with "
              << validPairs.size() << " valid block/rotation pairs." << std::endl;
}

bool BlockGenerator::isBlockValidAtPosition(int x, int y, int z, int blockId, int rotation) {
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
                        if (face == 2 || face == 3) { // +Y or -Y
                            std::cout << "Adjacency: Block " << blockId << " rot " << rotation << " face " << face
                            << " can connect to Block " << neighborId << " rot " << neighborRotation
                            << " face " << getOppositeFaceIndex(face) << std::endl;
}
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
                                                  const GridCell& neighborCell, int, int, int) {
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
    const std::vector<std::tuple<int, int, int>> neighbors = {
        {x, y, z+1}, {x, y, z-1}, {x+1, y, z}, {x-1, y, z}, {x, y+1, z}, {x, y-1, z}
    };
    for (const auto& [nx, ny, nz] : neighbors) {
        if (isValidGridPosition(nx, ny, nz) && !grid[nx][ny][nz].collapsed)
            updateCellPossibilities(nx, ny, nz);
    }
}

BlockMesh* BlockGenerator::generateMeshFromGrid() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<Texture>> textures;
    std::set<std::shared_ptr<Texture>> uniqueTextures;
    BlockMesh* blockMesh = new BlockMesh(vertices, indices, parameters, textures);
    unsigned int numThreads = 1;
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
    std::cout << "Initializing block weights..." << std::endl;
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
