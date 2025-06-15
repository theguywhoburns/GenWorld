#ifndef TERRAINGENERATOR_H
#define TERRAINGENERATOR_H

#include "IGeneratorStrategy.h"
#include "../Drawables/TerrainMesh.h"
#include "../Core/TerrainData.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>

class TerrainGenerator : public IGeneratorStrategy {
public:
    void Generate() override;
    std::vector<float> GenerateHeightMap();
    Mesh* GenerateFromHeightMap(const std::vector<float>& heightMap);
    void GenerateDecorations();

    Mesh* GetMesh() const override { return terrainMesh; }
    std::vector<float> GetHeightMap() const { return heightMap; }
    void SetParameters(const TerrainUtilities::TerrainData& params);

    struct ThreadTask {
        unsigned int startI, endI; // Range of columns (length) to process
        unsigned int startJ, endJ; // Range of rows (width) to process
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
    };

private:
    TerrainUtilities::TerrainData parameters;
    Mesh* terrainMesh = nullptr;
    std::vector<float> heightMap;

    float PerlinNoise(float x, float z);
    void updateSeedOffset();
    void CalculateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

};

#endif