#pragma once
#include <vector>
#include <string>
#include <map>
#include "../SocketSystem/SocketSystem.h"

namespace BlockUtilities {
    
    // Special block type IDs
    const int VOID_BLOCK_ID = -1;  // Represents empty space
    const int AIR_BLOCK_ID = -2;   // Represents air/ungenerated space
    
    struct BlockFaceConstraints {
        std::vector<int> validConnections;
        bool canBeExposed = true;
    };
    
    struct BlockConstraints {
        int blockId;
        BlockFaceConstraints posZ;    // +Z face
        BlockFaceConstraints negZ;    // -Z face
        BlockFaceConstraints posX;    // +X face
        BlockFaceConstraints negX;    // -X face
        BlockFaceConstraints posY;    // +Y face (top)
        BlockFaceConstraints negY;    // -Y face (bottom)
    };
    
    // Block generation constraints and weights
    struct BlockGenerationSettings {
        std::map<int, int> maxBlockCounts;        // blockId -> max count
        std::map<int, int> currentBlockCounts;    // blockId -> current count
        std::map<int, float> blockWeights;        // blockId -> weight (0.0-1.0)
        bool enforceBlockLimits = true;
        bool useWeightedSelection = true;
        float defaultWeight = 0.5f;               // Default weight for new blocks
    };
    
    struct BlockData {
        unsigned int gridWidth = 20;
        unsigned int gridHeight = 10;
        unsigned int gridLength = 20;
        
        float cellWidth = 5.0f;
        float cellHeight = 5.0f;
        float cellLength = 5.0f;
        
        float blockScale = 1.0f;
        
        // Void generation settings
        float voidProbability = 0.3f;  // 30% chance for void cells
        bool enableVoidCells = true;
        
        // World dimensions (calculated)
        float worldWidth = 0.0f;
        float worldHeight = 0.0f;
        float worldLength = 0.0f;
        float halfWorldWidth = 0.0f;
        float halfWorldLength = 0.0f;
        
        // Auto-detected dimensions from assets
        bool dimensionsDetected = false;
        float detectedBlockWidth = 0.0f;
        float detectedBlockHeight = 0.0f;
        float detectedBlockLength = 0.0f;
        
        BlockGenerationSettings generationSettings;
        
        // Add socket system
        SocketSystem socketSystem;
        std::map<int, int> blockRotations; // blockId -> current rotation
        bool useSocketSystem = true;
    };
    
} // namespace BlockUtilities