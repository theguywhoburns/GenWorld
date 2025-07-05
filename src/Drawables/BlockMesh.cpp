#include "BlockMesh.h"
#include "../Core/ShaderManager.h"
#include <iostream>

BlockMesh::BlockMesh(vector<Vertex> vertices, vector<unsigned int> indices, 
                     BlockUtilities::BlockData blockData,
                     vector<std::shared_ptr<Texture>> textures) 
    : Mesh(vertices, indices, textures) {
    
    this->data = blockData;
}

void BlockMesh::Draw(Shader& shader) {
}

void BlockMesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
    // Draw all block instances
    DrawBlockInstances(view, projection);
}

void BlockMesh::AddBlockInstance(int blockTypeId, const Transform& blockTransform) {
    blockInstances[blockTypeId].push_back(blockTransform.getModelMatrix());
}

void BlockMesh::AddBlockInstance(const std::string& assetPath, const Transform& blockTransform) {
    // Load model if not already loaded
    if (assetModels.find(assetPath) == assetModels.end()) {
        std::shared_ptr<Model> model = std::make_shared<Model>(assetPath.c_str());
        if (model) {
            assetModels[assetPath] = model;
        } else {
            std::cerr << "Failed to load block model: " << assetPath << std::endl;
            return;
        }
    }
    
    assetInstances[assetPath].push_back(blockTransform.getModelMatrix());
}

glm::vec3 BlockMesh::GetBlockPosition(int gridX, int gridZ) const {
    if (!IsValidGridPosition(gridX, gridZ)) {
        return glm::vec3(0.0f);
    }
    
    float worldX = (gridX * data.cellWidth) - data.halfWorldWidth;
    float worldZ = (gridZ * data.cellLength) - data.halfWorldLength;
    
    return glm::vec3(worldX, 0.0f, worldZ);
}

bool BlockMesh::IsValidGridPosition(int gridX, int gridZ) const {
    return gridX >= 0 && gridX < (int)data.gridWidth && 
           gridZ >= 0 && gridZ < (int)data.gridLength;
}

void BlockMesh::DrawBlockInstances(const glm::mat4& view, const glm::mat4& projection) {
    for (const auto& pair : assetInstances) {
        const std::string& assetPath = pair.first;
        const std::vector<glm::mat4>& instances = pair.second;
        
        if (instances.empty()) continue;
        
        std::shared_ptr<Model> model = assetModels[assetPath];
        if (model) {
            model->SetShaderParameters(m_currentShadingParams);
            model->DrawInstanced(view, projection, instances);
        }
    }
}
