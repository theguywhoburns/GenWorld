#ifndef TERRAINGENERATOR_H
#define TERRAINGENERATOR_H

#include "IGeneratorStrategy.h"
#include "../Drawables/Mesh.h"
#include "../Core/TerrainData.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

class TerrainGenerator : public IGeneratorStrategy {
public:
    Mesh* Generate() override;
    void SetParameters(const TerrainUtilities::TerrainData& params);

    struct ThreadTask {
        unsigned int startI, endI; // Range of rows (length) to process
        unsigned int startJ, endJ; // Range of columns (width) to process
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
    };

private:
    TerrainUtilities::TerrainData parameters;

    float PerlinNoise(float x, float z);
    void updateSeedOffset();
    glm::vec3 getColor(float height);

};

#endif