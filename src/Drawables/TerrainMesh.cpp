#include "TerrainMesh.h"
#include "../Core/stb_image_write.h"

TerrainMesh::TerrainMesh(vector<Vertex> vertices, vector<unsigned int> indices, TerrainUtilities::TerrainData terrainData,
    vector<float> heightMap) : Mesh(vertices, indices, vector<std::shared_ptr<Texture>>()) {

    this->data = terrainData;
    this->heightMap = heightMap;

    m_shader = ShaderManager::GetInstance()->getShader("terrain");
    textureShader = ShaderManager::GetInstance()->getShader("terrainTexture");

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

    RenderToTexture();
}

TerrainMesh::~TerrainMesh() {
    glDeleteTextures(1, &heightmapTextureID);
}

void TerrainMesh::Draw(Shader& shader) {
    // Activate the textures
    std::vector<TerrainUtilities::TextureData> loadedTextures = data.loadedTextures;

    shader.setInt("textureCount", loadedTextures.size());
    shader.setBool("coloringMode", data.coloringMode);

    for (int i = 0; i < loadedTextures.size(); i++) {
        Texture::activate(GL_TEXTURE0 + i);

        std::string name = "loadedTextures[" + std::to_string(i) + "]";
        shader.setInt(name + ".texture", i);
        shader.setFloat(name + ".height", loadedTextures[i].height);
        shader.setVec2(name + ".tiling", loadedTextures[i].tiling);
        shader.setVec2(name + ".offset", loadedTextures[i].offset);

        loadedTextures[i].texture->bind();
    }

    shader.setInt("colorCount", data.colors.size());
    for (int i = 0; i < data.colors.size(); i++) {
        std::string name = "colors[" + std::to_string(i) + "]";
        shader.setFloat(name + ".height", data.colors[i].height);
        shader.setVec4(name + ".color", data.colors[i].color);
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

    // Preview with ImGui (Just for testing)
    ImGui::Begin("Terrain Texture Preview");
    ImGui::Image((intptr_t)resultTextureID, ImVec2(512, 512));
    ImGui::End();
}

void TerrainMesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
    if (m_shader != nullptr) {
        m_shader->use();
        glm::mat4 model = transform.getModelMatrix();
        m_shader->setMat4("model", model);
        m_shader->setMat4("view", view);
        m_shader->setMat4("projection", projection);
        Draw(*m_shader);

        // Draw instances
        DrawInstances(view, projection);
    }
}

void TerrainMesh::AddInstance(const std::string& modelPath, const Transform& transform) {
    // Check if the model is already loaded
    if (instanceMeshes.find(modelPath) == instanceMeshes.end()) {
        // Load the model if not already loaded
        std::shared_ptr<Model> model = std::make_shared<Model>(modelPath.c_str());
        if (model) {
            instanceMeshes[modelPath] = model;
        }
        else {
            std::cerr << "Model not found: " << modelPath << std::endl;
            return;
        }
    }

    modelInstances[modelPath].push_back(transform.getModelMatrix());
}

void TerrainMesh::RenderToTexture() {
    textureShader->use();

    FrameBuffer frameBuffer;
    frameBuffer.Resize(1024, 1024);

    frameBuffer.bind();
    glViewport(0, 0, 1024, 1024);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Draw the terrain mesh to the framebuffer
    Draw(*textureShader);


    // Read the pixels from the framebuffer
    unsigned char* pixels = new unsigned char[1024 * 1024 * 4];
    glReadPixels(0, 0, 1024, 1024, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    frameBuffer.unbind();

    // Create a texture for the result
    glDeleteTextures(1, &resultTextureID);
    glGenTextures(1, &resultTextureID);
    glBindTexture(GL_TEXTURE_2D, resultTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] pixels;

    // Unbind the framebuffer
    frameBuffer.Destroy();
}

void TerrainMesh::DrawInstances(const glm::mat4& view, const glm::mat4& projection) {
    for (const auto& pair : modelInstances) {
        const std::string& modelPath = pair.first;
        const std::vector<glm::mat4>& instances = pair.second;

        if(instances.empty()) continue;

        std::shared_ptr<Model> model = instanceMeshes[modelPath];
        if (model) {
            model->SetShader(ShaderManager::GetInstance()->getShader("unshaded"));  // TODO: change this to the appropriate shader if needed according to the viewport render mode
            model->DrawInstanced(view, projection, instances);
        }
    }
}

float TerrainMesh::GetHeightAt(float x, float z) const {
    int gridX = static_cast<int>((x + data.halfWidth) / data.stepX);
    int gridZ = static_cast<int>((z + data.halfLength) / data.stepZ);

    if (gridX < 0 || gridX >= data.numCellsWidth || gridZ < 0 || gridZ >= data.numCellsLength) {
        return 0.0f;
    }

    return heightMap[gridZ * data.numCellsWidth + gridX];
}