#pragma once
#include <vector>
#include <string>
#include <map>
#include <set>
#include "../SocketSystem/SocketSystem.h"

namespace BlockUtilities {
    
    // Block generation constraints and weights
    struct BlockGenerationSettings {
        std::map<int, int> maxBlockCounts;     // blockId -> max count
        std::map<int, int> minBlockCounts;     // blockId -> min count
        std::map<int, int> currentBlockCounts;  // blockId -> current count
        std::map<int, float> blockWeights;      // blockId -> weight (0.0-1.0)
        bool enforceBlockLimits = true;
        bool useWeightedSelection = true;
        float defaultWeight = 0.5f;               // Default weight for new blocks
        bool isGridMaskEnabled = false;
        std::set<int> cornerBlockIds;
    };
    
    struct BlockData {
        // Constructor with defaults and optional arguments
        BlockData(
            unsigned int gridWidth_ = 20,
            unsigned int gridHeight_ = 10,
            unsigned int gridLength_ = 20,
            float cellWidth_ = 5.0f,
            float cellHeight_ = 5.0f,
            float cellLength_ = 5.0f,
            float blockScale_ = 1.0f,
            float gridScale_ = 1.0f,
            unsigned long randomSeed_ = 12345
        )
            : gridWidth(gridWidth_),
              gridHeight(gridHeight_),
              gridLength(gridLength_),
              cellWidth(cellWidth_),
              cellHeight(cellHeight_),
              cellLength(cellLength_),
              randomSeed(randomSeed_),
              blockScale(blockScale_),
              gridScale(gridScale_)
        {}
    
        unsigned int gridWidth = 20;
        unsigned int gridHeight = 10;
        unsigned int gridLength = 20;
    
        float cellWidth = 5.0f;
        float cellHeight = 5.0f;
        float cellLength = 5.0f;
    
        unsigned int randomSeed = 12345; // Seed for random number generation
    
        float blockScale = 1.0f;
        float gridScale = 1.0f;
  
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
    };
} // namespace BlockUtilities