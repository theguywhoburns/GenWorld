#include "TerrainMesh.h"
#include "../Core/stb_image_write.h"

TerrainMesh::TerrainMesh(vector<Vertex> vertices, vector<unsigned int> indices, TerrainUtilities::TerrainData terrainData
    , vector<float> heightMap)
    : Mesh(vertices, indices, vector<Texture>()) {

    this->data = terrainData;
    this->heightMap = heightMap;

    // Create a texture for the height map
    glGenTextures(1, &heightmapTextureID);
    glBindTexture(GL_TEXTURE_2D, heightmapTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, terrainData.numCellsWidth, terrainData.numCellsLength, 0, GL_RED, GL_FLOAT, heightMap.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

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

    // Bind the height map texture
    Texture::activate(GL_TEXTURE0 + loadedTextures.size());
    shader.setInt("heightmap", loadedTextures.size());
    glBindTexture(GL_TEXTURE_2D, heightmapTextureID);

    glBindVertexArray(arrayObj);
    glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    Texture::activate(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
