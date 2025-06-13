#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Transform {
private:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    mutable glm::mat4 modelMatrix = glm::mat4(1.0f);
    mutable bool isDirty = true;

public:
    Transform() = default;
    Transform(float x, float y, float z) : position(x, y, z) {}
    Transform(float x, float y, float z, float rx, float ry, float rz) 
        : position(x, y, z), rotation(rx, ry, rz) {}
    Transform(const glm::vec3& pos) : position(pos) {}
    Transform(const glm::vec3& pos, const glm::vec3& rot) : position(pos), rotation(rot) {}
    Transform(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl) 
        : position(pos), rotation(rot), scale(scl) {}

    // Position methods
    void setPosition(float x, float y, float z) {
        position = glm::vec3(x, y, z);
        isDirty = true;
    }
    
    void setPosition(const glm::vec3& pos) {
        position = pos;
        isDirty = true;
    }
    
    const glm::vec3& getPosition() const {
        return position;
    }

    // Rotation methods (in degrees)
    void setRotation(float rx, float ry, float rz) {
        rotation = glm::vec3(rx, ry, rz);
        isDirty = true;
    }
    
    void setRotation(const glm::vec3& rot) {
        rotation = rot;
        isDirty = true;
    }
    
    const glm::vec3& getRotation() const {
        return rotation;
    }

    // Scale methods
    void setScale(float sx, float sy, float sz) {
        scale = glm::vec3(sx, sy, sz);
        isDirty = true;
    }
    
    void setScale(const glm::vec3& scl) {
        scale = scl;
        isDirty = true;
    }
    
    void setScale(float uniformScale) {
        scale = glm::vec3(uniformScale);
        isDirty = true;
    }
    
    const glm::vec3& getScale() const {
        return scale;
    }

    void translate(const glm::vec3& translation) {
        position += translation;
        isDirty = true;
    }
    
    void rotate(const glm::vec3& rotationDelta) {
        rotation += rotationDelta;
        isDirty = true;
    }
    
    void scaleBy(const glm::vec3& scaleDelta) {
        scale *= scaleDelta;
        isDirty = true;
    }
    
    void scaleBy(float uniformScale) {
        scale *= uniformScale;
        isDirty = true;
    }

    // Matrix generation
    const glm::mat4& getModelMatrix() const {
        if (isDirty) {
            updateModelMatrix();
        }
        return modelMatrix;
    }
    
    glm::mat4 getTranslationMatrix() const {
        return glm::translate(glm::mat4(1.0f), position);
    }
    
    glm::mat4 getRotationMatrix() const {
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        return rotZ * rotY * rotX;
    }
    
    glm::mat4 getScaleMatrix() const {
        return glm::scale(glm::mat4(1.0f), scale);
    }

    glm::vec3 getForward() const {
        glm::mat4 rotMat = getRotationMatrix();
        return glm::normalize(glm::vec3(rotMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    }
    
    glm::vec3 getRight() const {
        glm::mat4 rotMat = getRotationMatrix();
        return glm::normalize(glm::vec3(rotMat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
    }
    
    glm::vec3 getUp() const {
        glm::mat4 rotMat = getRotationMatrix();
        return glm::normalize(glm::vec3(rotMat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
    }

    // Reset to identity
    void reset() {
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        isDirty = true;
    }

    Transform operator*(const Transform& other) const {
        Transform result;
        result.modelMatrix = getModelMatrix() * other.getModelMatrix();
        result.isDirty = false;
        return result;
    }

private:
    void updateModelMatrix() const {
        // Order: Scale -> Rotate -> Translate (TRS)
        modelMatrix = getTranslationMatrix() * getRotationMatrix() * getScaleMatrix();
        isDirty = false;
    }
};