#include <BufferObject.h>

BufferObject::BufferObject(GLenum target, const void* data, GLsizeiptr size, GLenum usage) {
    this->target = target;
    glGenBuffers(1, &ID);
    bind();
    glBufferData(target, size, data, usage);
    unbind();
}

void BufferObject::bind() {
    glBindBuffer(target, ID);
}

void BufferObject::unbind() {
    glBindBuffer(target, 0);
}

void BufferObject::deleteBuffer() {
    glDeleteBuffers(1, &ID);
}

void BufferObject::updateData(GLintptr offset, GLsizeiptr size, const void* data) {
    glBufferSubData(target, offset, size, data);
}
