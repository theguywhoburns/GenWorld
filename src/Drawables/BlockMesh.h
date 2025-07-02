#pragma once

#include "Mesh.h"
#include "Model.h"
#include "../Core/BlockData.h"
#include "../Core/FrameBuffer.h"
#include <unordered_map>

class BlockMesh : public Mesh {
public:
    BlockMesh(vector<Vertex> vertices, vector<unsigned int> indices, 
              BlockUtilities::BlockData blockData,
              vector<std::shared_ptr<Texture>> textures = vector<std::shared_ptr<Texture>>());
    ~BlockMesh();

    void Draw(Shader& shader) override;
    void Draw(const glm::mat4& view, const glm::mat4& projection) override;

    // Block-specific methods
    void AddBlockInstance(int blockTypeId, const Transform& transform);
    void AddBlockInstance(const std::string& assetPath, const Transform& transform);
    void ClearInstances();
    
    // Utility methods
    glm::vec3 GetBlockPosition(int gridX, int gridZ) const;
    bool IsValidGridPosition(int gridX, int gridZ) const;
    
    // Texture management for block textures
    void SetBlockTextures(const std::vector<std::shared_ptr<Texture>>& blockTextures);

private:
    BlockUtilities::BlockData data;
    
    // Instance management (similar to TerrainMesh)
    std::unordered_map<int, std::shared_ptr<Model>> blockModels;           // blockTypeId -> Model
    std::unordered_map<std::string, std::shared_ptr<Model>> assetModels;   // assetPath -> Model
    std::unordered_map<int, std::vector<glm::mat4>> blockInstances;        // blockTypeId -> transforms
    std::unordered_map<std::string, std::vector<glm::mat4>> assetInstances; // assetPath -> transforms
    
    // Texture management
    std::vector<std::shared_ptr<Texture>> blockTextures;
    unsigned int blockTextureArrayID = 0;
    
    // Rendering
    void DrawBlockInstances(const glm::mat4& view, const glm::mat4& projection);
    
    // Preview/Debug
    unsigned int previewTextureID = 0;
    void RenderBlockPreview();
};