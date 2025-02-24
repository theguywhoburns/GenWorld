#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include <Drawables/IDrawable.h>
#include <Texture.h>
#include <BufferObject.h>
#include <VAO.h>
#include <Vertex.h>

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
private:
    unsigned int arrayObj, vertexBuffer, indexBuffer;

    void setupMesh();
};

#endif