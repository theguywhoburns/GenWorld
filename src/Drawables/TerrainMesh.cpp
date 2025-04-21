#include "TerrainMesh.h"

void TerrainMesh::Draw(Shader& shader) {
    // Activate the textures
    shader.setInt("textureCount", textures.size());
    for (unsigned int i = 0; i < textures.size(); i++) {
        Texture::activate(GL_TEXTURE0 + i);
        shader.setInt("textureSampler[" + std::to_string(i) + "]", i);
        textures[i].bind();
    }

    // Set the blending factor and heights for the shader
    shader.setFloat("blendingFactor", blendingFactor);
    for (unsigned int i = 0; i < heights.size(); i++) {
        shader.setFloat("heights[" + std::to_string(i) + "]", heights[i]);
    }

    glBindVertexArray(arrayObj);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    Texture::activate(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}