#include <vector>
#include <glm/glm.hpp>

class Grid {
    private:
        int width;
        int height;
        int depth;
        float cellSize;
        std::vector<int> cells;
    public:
        Grid(int width, int height, int depth, float cellSize);
        void setCell(int x, int y, int z, int value);
        int getCell(int x, int y, int z) const;
        void clear();
        
        glm::vec3 gridToWorld(int x, int y, int z) const;
        glm::ivec3 worldToGrid(const glm::vec3& worldPos) const;


        glm::ivec3 getDimenstions() const;

};