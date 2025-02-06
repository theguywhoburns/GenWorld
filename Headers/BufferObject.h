#ifndef BUFFEROBJECT_H
#define BUFFEROBJECT_H

#include <glad/glad.h>

class BufferObject {
public:
    unsigned int ID;
    GLenum target;
    BufferObject(GLenum target, const void* data, GLsizeiptr size, GLenum usage);
    // Binds the VBO
    void bind();
    // Unbinds the VBO
    void unbind();
    // Deletes the VBO
    void deleteBuffer();
    // Updates a portion of the buffer's data store
    void updateData(GLintptr offset, GLsizeiptr size, const void* data);
};

#endif