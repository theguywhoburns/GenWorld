#ifndef TERRAINGENERATOR_H
#define TERRAINGENERATOR_H

#include "Generators/IGeneratorStrategy.h"
#include "Drawables/Mesh.h"
#include "TerrainData.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

class TerrainGenerator : public IGeneratorStrategy {
public:
    Mesh* Generate() override;
    void SetParameters(const TerrainUtilities::TerrainData& params);

private:
    TerrainUtilities::TerrainData parameters;

    float PerlinNoise(float x, float z);
    void updateSeedOffset();
    glm::vec3 getColor(float height);

};

#endif