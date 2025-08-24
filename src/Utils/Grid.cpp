#include <GenWorld/Utils/Grid.h>

Grid::Grid(int width, int height, int depth, float cellSize) {
  this->width = width;
  this->height = height;
  this->depth = depth;
  this->cellSize = cellSize;
  cells.resize(width * height * depth, 0);
}

void Grid::setCell(int x, int y, int z, int value) {
  if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth) {
    cells[x + y * width + z * width * height] = value;
  }
}

int Grid::getCell(int x, int y, int z) const {
  if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth) {
    return cells[x + y * width + z * width * height];
  }
  return -1; // or some other error value
}

void Grid::clear() { std::fill(cells.begin(), cells.end(), 0); }

glm::vec3 Grid::gridToWorld(int x, int y, int z) const {
  return glm::vec3(x * cellSize + cellSize * 0.5f,
                   y * cellSize + cellSize * 0.5f,
                   z * cellSize + cellSize * 0.5f);
}

glm::ivec3 Grid::worldToGrid(const glm::vec3 &worldPos) const {
  return glm::ivec3(static_cast<int>(worldPos.x / cellSize),
                    static_cast<int>(worldPos.y / cellSize),
                    static_cast<int>(worldPos.z / cellSize));
}

glm::ivec3 Grid::getDimenstions() const {
  return glm::ivec3(width, height, depth);
}
