#pragma once

#include "Mesh.h"
#include "../Core/TerrainData.h"

class TerrainMesh : public Mesh {
public:
    TerrainMesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<TerrainUtilities::TextureData> textureData, float blendingFactor)
        : Mesh(vertices, indices, vector<Texture>()) {

        for (int i = 0; i < textureData.size(); i++) {
            textures.push_back(textureData[i].texture);
            heights.push_back(textureData[i].height);
        }

        this->blendingFactor = blendingFactor;
    }
    void Draw(Shader& shader) override;
private:
    vector<float> heights;
    float blendingFactor = 0.5f;

};