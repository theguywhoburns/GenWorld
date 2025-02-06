#include"VAO.h"

// Constructor that generates a VAO ID
VAO::VAO() {
    glGenVertexArrays(1, &ID);
}

// Links a VBO to the VAO using a certain layout
void VAO::linkVBO(BufferObject& VBO, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    bind();
    VBO.bind();
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    glEnableVertexAttribArray(index);
    VBO.unbind();
    unbind();
}

// Binds the VAO
void VAO::bind() {
    glBindVertexArray(ID);
}

// Unbinds the VAO
void VAO::unbind() {
    glBindVertexArray(0);
}

// Deletes the VAO
void VAO::deleteVAO() {
    glDeleteVertexArrays(1, &ID);
}