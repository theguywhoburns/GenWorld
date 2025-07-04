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

    void Draw(Shader& shader) override;
    void Draw(const glm::mat4& view, const glm::mat4& projection) override;

    // Block-specific methods
    void AddBlockInstance(int blockTypeId, const Transform& transform);
    void AddBlockInstance(const std::string& assetPath, const Transform& transform);
    
    // Utility methods
    glm::vec3 GetBlockPosition(int gridX, int gridZ) const;
    bool IsValidGridPosition(int gridX, int gridZ) const;

private:
    BlockUtilities::BlockData data;
    
    // Instance management - only asset path instances are actually rendered
    std::unordered_map<std::string, std::shared_ptr<Model>> assetModels;   // assetPath -> Model
    std::unordered_map<int, std::vector<glm::mat4>> blockInstances;      // blockTypeId -> transforms
    std::unordered_map<std::string, std::vector<glm::mat4>> assetInstances; // assetPath -> transforms
    
    // Texture management
    std::vector<std::shared_ptr<Texture>> blockTextures;
    unsigned int blockTextureArrayID = 0;
    
    // Rendering
    void DrawBlockInstances(const glm::mat4& view, const glm::mat4& projection);
};