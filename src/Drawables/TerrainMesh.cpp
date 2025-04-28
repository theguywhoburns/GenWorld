#include "TerrainMesh.h"

void TerrainMesh::Draw(Shader& shader) {
    // Activate the textures
    std::vector<TerrainUtilities::TextureData> loadedTextures = data.loadedTextures;

    shader.setInt("textureCount", loadedTextures.size());
    shader.setFloat("blendingFactor", data.blendFactor);
    shader.setBool("coloringMode", data.coloringMode);

    for (int i = 0; i < loadedTextures.size(); i++) {
        Texture::activate(GL_TEXTURE0 + i);

        std::string name = "loadedTextures[" + std::to_string(i) + "]";
        shader.setInt(name + ".texture", i);
        shader.setFloat(name + ".height", loadedTextures[i].height);
        shader.setVec2(name + ".tiling", loadedTextures[i].tiling);
        shader.setVec2(name + ".offset", loadedTextures[i].offset);

        loadedTextures[i].texture.bind();
    }

    glBindVertexArray(arrayObj);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    Texture::activate(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}