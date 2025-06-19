#pragma once

#include "Mesh.h"
#include "Model.h"
#include "../Core/TerrainData.h"
#include "../Core/FrameBuffer.h"

class TerrainMesh : public Mesh {
public:
    TerrainMesh(vector<Vertex> vertices, vector<unsigned int> indices, TerrainUtilities::TerrainData terrainData, vector<float> heightMap);
    ~TerrainMesh();

    void Draw(Shader& shader) override;
    void Draw(const glm::mat4& view, const glm::mat4& projection) override;

    void AddInstance(const std::string& modelPath, const Transform& transform);

    float GetHeightAt(float x, float z) const;

private:
    TerrainUtilities::TerrainData data;
    vector<float> heightMap;
    unsigned int heightmapTextureID = 0;

    unsigned int resultTextureID = 0;
    std::shared_ptr<Shader> textureShader;
    void RenderToTexture();

    std::unordered_map<std::string, std::shared_ptr<Model>> instanceMeshes;
    std::unordered_map<std::string, std::vector<glm::mat4>> modelInstances;
    void DrawInstances(const glm::mat4& view, const glm::mat4& projection);
};