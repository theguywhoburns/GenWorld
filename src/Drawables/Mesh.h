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

using namespace std;

class Mesh : public IDrawable {
public:
    // mesh data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;

    //Mesh() { setupMesh(); }
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures);
    ~Mesh();
    void Draw(Shader& shader) override;
    void Draw(const glm::mat4& view, const glm::mat4& projection) override;

protected:
    unsigned int arrayObj = 0;

private:
    unsigned int vertexBuffer, indexBuffer;

    void setupMesh();
};

#endif