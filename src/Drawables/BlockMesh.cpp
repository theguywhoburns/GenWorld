#include "BlockMesh.h"
#include "../Core/ShaderManager.h"
#include <iostream>

BlockMesh::BlockMesh(vector<Vertex> vertices, vector<unsigned int> indices, 
                     BlockUtilities::BlockData blockData,
                     vector<std::shared_ptr<Texture>> textures) 
    : Mesh(vertices, indices, textures) {
    
    this->data = blockData;
    this->blockTextures = textures;
    
    blockShader = ShaderManager::GetInstance()->getShader("block");
}

BlockMesh::~BlockMesh() {
    if (blockTextureArrayID != 0) {
        glDeleteTextures(1, &blockTextureArrayID);
    }
    if (previewTextureID != 0) {
        glDeleteTextures(1, &previewTextureID);
    }
}

void BlockMesh::Draw(Shader& shader) {
    // Set block-specific uniforms
    shader.setInt("gridWidth", data.gridWidth);
    shader.setInt("gridLength", data.gridLength);
    shader.setFloat("cellWidth", data.cellWidth);
    shader.setFloat("cellLength", data.cellLength);
    shader.setFloat("blockScale", data.blockScale);
    
    // Bind block textures
    if (!blockTextures.empty()) {
        shader.setInt("blockTextureCount", blockTextures.size());
        for (size_t i = 0; i < blockTextures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string uniformName = "blockTextures[" + std::to_string(i) + "]";
            shader.setInt(uniformName, i);
            blockTextures[i]->bind();
        }
        shader.setBool("hasBlockTextures", true);
    } else {
        shader.setBool("hasBlockTextures", false);
    }
    
    // Draw the base mesh (if any)
    if (!vertices.empty()) {
        glBindVertexArray(arrayObj);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    // Reset texture state
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void BlockMesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
    if (blockShader != nullptr) {
        blockShader->use();
        
        // Set transformation matrices
        glm::mat4 model = transform.getModelMatrix();
        blockShader->setMat4("model", model);
        blockShader->setMat4("view", view);
        blockShader->setMat4("projection", projection);
        
        // Draw base mesh
        Draw(*blockShader);
        
        // Draw all block instances
        DrawBlockInstances(view, projection);
    }
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

void BlockMesh::ClearInstances() {
    blockInstances.clear();
    assetInstances.clear();
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

void BlockMesh::SetBlockTextures(const std::vector<std::shared_ptr<Texture>>& textures) {
    blockTextures = textures;
}

void BlockMesh::DrawBlockInstances(const glm::mat4& view, const glm::mat4& projection) {
    // Draw instances by block type ID
    for (const auto& pair : blockInstances) {
        int blockTypeId = pair.first;
        const std::vector<glm::mat4>& instances = pair.second;
        
        if (instances.empty()) continue;
        
        // Use model if available for this block type
        if (blockModels.find(blockTypeId) != blockModels.end()) {
            std::shared_ptr<Model> model = blockModels[blockTypeId];
            if (model) {
                model->SetShader(blockShader);
                model->DrawInstanced(view, projection, instances);
            }
        }
    }
    
    // Draw instances by asset path
    for (const auto& pair : assetInstances) {
        const std::string& assetPath = pair.first;
        const std::vector<glm::mat4>& instances = pair.second;
        
        if (instances.empty()) continue;
        
        std::shared_ptr<Model> model = assetModels[assetPath];
        if (model) {
            model->SetShader(blockShader);
            model->DrawInstanced(view, projection, instances);
        }
    }
}

void BlockMesh::RenderBlockPreview() {
    // Similar to TerrainMesh::RenderToTexture() but for block preview
    if (!blockShader) return;
    
    blockShader->use();
    
    FrameBuffer frameBuffer;
    frameBuffer.Resize(512, 512);
    
    frameBuffer.bind();
    glViewport(0, 0, 512, 512);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render blocks from top-down view
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 50, 0),  // Camera above the blocks
        glm::vec3(0, 0, 0),   // Looking at center
        glm::vec3(0, 0, -1)   // Up direction
    );
    glm::mat4 projection = glm::ortho(-25.0f, 25.0f, -25.0f, 25.0f, 0.1f, 100.0f);
    
    Draw(view, projection);
    
    // Read pixels and create preview texture
    unsigned char* pixels = new unsigned char[512 * 512 * 4];
    glReadPixels(0, 0, 512, 512, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    frameBuffer.unbind();
    
    // Create preview texture
    if (previewTextureID != 0) {
        glDeleteTextures(1, &previewTextureID);
    }
    
    glGenTextures(1, &previewTextureID);
    glBindTexture(GL_TEXTURE_2D, previewTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    delete[] pixels;
    frameBuffer.Destroy();
}