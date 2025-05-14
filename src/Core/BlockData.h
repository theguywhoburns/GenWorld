namespace BlockUtilities {
    enum BlockConstraint {
        LEFT,
        RIGHT,
        FRONT,
        BACK,
        UP,
        DOWN,
    };

    struct BlockData {
        // Block Data
        int id;
        vector<BlockConstraint> constraints;
        std::string blockPath;
        std::string texturePath;
        float width;
        float length;
        float height;
        int cellSize;
        unsigned int numCellsWidth;
        unsigned int numCellsLength;
        unsigned int numCellsHeight;
        float halfWidth;
        float halfLength;
        float halfHeight;
        float stepX;
        float stepY;
        float stepZ;
    };
}