#pragma once  // Add header guard

#include <vector>
#include <string>

namespace BlockUtilities {
    struct BlockConstraints {
        int blockTypeId;
        std::vector<int> allowedNeighbors;
        std::string blockTypeName;
        
        BlockConstraints() : blockTypeId(-1) {}
        BlockConstraints(int id, const std::string& name) : blockTypeId(id), blockTypeName(name) {}
    };

    struct BlockData {
        // Grid parameters
        float width;
        float length;
        int cellSize;
        unsigned int numCellsWidth;
        unsigned int numCellsLength;
        float halfWidth;
        float halfLength;
        float stepX;
        float stepZ;
        

        float blockScale = 1.0f;
        
        std::vector<BlockConstraints> blockConstraints;
    };
}