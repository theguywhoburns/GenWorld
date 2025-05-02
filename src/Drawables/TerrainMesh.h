#pragma once

#include "Mesh.h"
#include "../Core/TerrainData.h"
#include "../Core/FrameBuffer.h"

class TerrainMesh : public Mesh {
public:
    TerrainMesh(vector<Vertex> vertices, vector<unsigned int> indices, TerrainUtilities::TerrainData terrainData, vector<float> heightMap);
    void Draw(Shader& shader) override;

private:
    TerrainUtilities::TerrainData data;
    vector<float> heightMap;
    Texture* heightMapTexture = nullptr;

};