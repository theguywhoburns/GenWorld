#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Transform {
private:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f); // In degrees
    glm::vec3 scale = glm::vec3(1.0f);
    
    mutable glm::mat4 modelMatrix = glm::mat4(1.0f);
    mutable bool isDirty = true;

    void markDirty() { isDirty = true; }

public:
    // Constructors
    Transform() = default;
    Transform(const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.0f), const glm::vec3& scl = glm::vec3(1.0f)) 
        : position(pos), rotation(rot), scale(scl) {}
    Transform(float x, float y, float z, float rx = 0.0f, float ry = 0.0f, float rz = 0.0f) 
        : position(x, y, z), rotation(rx, ry, rz) {}

    void setPosition(const glm::vec3& pos) { position = pos; markDirty(); }
    void setPosition(float x, float y, float z) { setPosition(glm::vec3(x, y, z)); }
    
    void setRotation(const glm::vec3& rot) { rotation = rot; markDirty(); }
    void setRotation(float rx, float ry, float rz) { setRotation(glm::vec3(rx, ry, rz)); }
    
    void setScale(const glm::vec3& scl) { scale = scl; markDirty(); }
    void setScale(float sx, float sy, float sz) { setScale(glm::vec3(sx, sy, sz)); }
    void setScale(float uniformScale) { setScale(glm::vec3(uniformScale)); }

    const glm::vec3& getPosition() const { return position; }
    const glm::vec3& getRotation() const { return rotation; }
    const glm::vec3& getScale() const { return scale; }

    void translate(const glm::vec3& delta) { position += delta; markDirty(); }
    void rotate(const glm::vec3& delta) { rotation += delta; markDirty(); }
    void scaleBy(const glm::vec3& delta) { scale *= delta; markDirty(); }
    void scaleBy(float uniformScale) { scale *= uniformScale; markDirty(); }

    const glm::mat4& getModelMatrix() const {
        if (isDirty) {
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, position);
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
            modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
            modelMatrix = glm::scale(modelMatrix, scale);
            isDirty = false;
        }
        return modelMatrix;
    }

    void reset() {
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        markDirty();
    }

    Transform operator*(const Transform& other) const {
        Transform result;
        result.modelMatrix = getModelMatrix() * other.getModelMatrix();
        result.isDirty = false;
        return result;
    }
};