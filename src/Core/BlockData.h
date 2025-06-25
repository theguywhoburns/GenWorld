#pragma once
#include <vector>
#include <string>

namespace BlockUtilities {
    
    struct BlockData {
        unsigned int gridWidth = 20;
        unsigned int gridHeight = 10;    // Added height for 3D generation
        unsigned int gridLength = 20;
        
        float cellWidth = 5.0f;
        float cellHeight = 5.0f;         // Added height dimension
        float cellLength = 5.0f;
        
        float blockScale = 1.0f;
        
        // World dimensions (calculated)
        float worldWidth = 0.0f;
        float worldHeight = 0.0f;        // Added world height
        float worldLength = 0.0f;
        float halfWorldWidth = 0.0f;
        float halfWorldLength = 0.0f;
        
        // Auto-detected dimensions from assets
        bool dimensionsDetected = false;
        float detectedBlockWidth = 0.0f;
        float detectedBlockHeight = 0.0f;    // Added detected height
        float detectedBlockLength = 0.0f;
    };
    
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
    
} // namespace BlockUtilities