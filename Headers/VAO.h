#ifndef VAO_H
#define VAO_H

#include <glad/glad.h>
#include <BufferObject.h>

class VAO {
public:
    // ID reference for the Vertex Array Object
    unsigned int ID;
    // Constructor that generates a VAO ID
    VAO();
    // Links a VBO to the VAO using a certain layout
    void linkVBO(BufferObject& VBO, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
    // Binds the VAO
    void bind();
    // Unbinds the VAO
    void unbind();
    // Deletes the VAO
    void deleteVAO();
};
#endif