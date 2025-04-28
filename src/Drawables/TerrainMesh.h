#pragma once

#include "Mesh.h"
#include "../Core/TerrainData.h"

class TerrainMesh : public Mesh {
public:
    TerrainMesh(vector<Vertex> vertices, vector<unsigned int> indices, TerrainUtilities::TerrainData terrainData)
        : Mesh(vertices, indices, vector<Texture>()) {

        this->data = terrainData;
    }
    void Draw(Shader& shader) override;
private:
    TerrainUtilities::TerrainData data;

};