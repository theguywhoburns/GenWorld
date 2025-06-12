#pragma once
#include <vector>
#include <string>
#include <map>

namespace BlockUtilities {
    // Enum for block sides/directions
    enum class BlockSide {
        FRONT = 0,   // +Z
        BACK = 1,    // -Z
        LEFT = 2,    // -X
        RIGHT = 3,   // +X
        TOP = 4,     // +Y
        BOTTOM = 5   // -Y
    };

    // Structure to define directional constraints
    struct DirectionalConstraint {
        int blockTypeId;                                        
        std::map<BlockSide, std::vector<int>> allowedNeighbors; 
        std::string blockTypeName;                              
        
        DirectionalConstraint() : blockTypeId(-1) {}
        DirectionalConstraint(int id, const std::string& name) : blockTypeId(id), blockTypeName(name) {}
    };

    struct BlockData {
        unsigned int gridWidth = 20;      // Number of blocks in X direction
        unsigned int gridLength = 20;     // Number of blocks in Z direction
        
        // Calculated world dimensions (auto-calculated from block dimensions)
        float worldWidth;
        float worldLength;
        float halfWorldWidth;
        float halfWorldLength;
        
        // Cell/Block dimensions (auto-detected)
        float cellWidth = 5.0f;           // Width of each cell/block
        float cellLength = 5.0f;          // Length of each cell/block
        
        // Block appearance
        float blockScale = 1.0f;
        
        // Auto-detected block dimensions
        float detectedBlockWidth = 0.0f;
        float detectedBlockLength = 0.0f;
        float detectedBlockHeight = 0.0f;
        bool dimensionsDetected = false;
        
        // Directional constraints for adjacency rules
        std::vector<DirectionalConstraint> directionalConstraints;
    };
}