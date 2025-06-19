#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "IDrawable.h"
#include "../Core/Texture.h"
#include "../Core/Vertex.h"
#include "../Core/Engine/Transform.h"

using namespace std;

class Mesh : public IDrawable {
public:
    // mesh data
    vector<Vertex>                      vertices;
    vector<unsigned int>                indices;
    vector<std::shared_ptr<Texture>>    textures;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<std::shared_ptr<Texture>> textures);
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<std::shared_ptr<Texture>> textures, const Transform& initialTransform);
    virtual ~Mesh();
    
    // Drawing methods
    void Draw(Shader& shader) override;
    void Draw(const glm::mat4& view, const glm::mat4& projection) override;
    void Draw(Shader& shader, const glm::mat4& parentTransform);

    void DrawInstanced(unsigned int instanceCount, const glm::mat4& view, const glm::mat4& projection);
    void InitializeInstanceBuffer();
    void UpdateInstanceData(const std::vector<glm::mat4>& instanceMatrices);

protected:
    unsigned int arrayObj = 0;

private:
    unsigned int vertexBuffer, indexBuffer;

    unsigned int instanceVBO = 0;
    bool instancingInitialized = false;

    void setupMesh();
    void bindTextures(Shader& shader);
    void unbindTextures();

};

#endif