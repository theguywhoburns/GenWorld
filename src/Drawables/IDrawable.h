#pragma once

#include "../Core/Shader.h"
#include "../Core/ShaderManager.h"
#include "../Core/Engine/Transform.h"
#include <memory>

class IDrawable {
protected:
    std::shared_ptr<Shader> m_shader;
    std::string m_solidShader;
    std::string m_unShadedShader;

    Transform transform;



public:
    IDrawable() {
        m_solidShader = "solid";
        m_unShadedShader = "unshaded";
        m_shader = ShaderManager::GetInstance()->getShader(m_solidShader);

        transform = Transform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    }
    virtual ~IDrawable() = default;

    virtual void Draw(Shader& shader) = 0;
    virtual void Draw(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void SetShader(std::shared_ptr<Shader> shader) { m_shader = shader; }
    virtual std::shared_ptr<Shader> GetShader() { return m_shader; }

    void setPosition(const glm::vec3& position) { transform.setPosition(position); }
    void setPosition(float x, float y, float z) { transform.setPosition(x, y, z); }
    void setRotation(const glm::vec3& rotation) { transform.setRotation(rotation); }
    void setRotation(float rx, float ry, float rz) { transform.setRotation(rx, ry, rz); }
    void setScale(const glm::vec3& scale) { transform.setScale(scale); }
    void setScale(float uniformScale) { transform.setScale(uniformScale); }
    void setScale(float sx, float sy, float sz) { transform.setScale(sx, sy, sz); }
    
    const glm::vec3& getPosition() const { return transform.getPosition(); }
    const glm::vec3& getRotation() const { return transform.getRotation(); }
    const glm::vec3& getScale() const { return transform.getScale(); }
    const glm::mat4& getModelMatrix() const { return transform.getModelMatrix(); }

    void translate(const glm::vec3& translation) { transform.translate(translation); }
    void rotate(const glm::vec3& rotation) { transform.rotate(rotation); }
    void scaleBy(const glm::vec3& scale) { transform.scaleBy(scale); }
    void scaleBy(float uniformScale) { transform.scaleBy(uniformScale); }

    glm::vec3 getForward() const { return transform.getForward(); }
    glm::vec3 getRight() const { return transform.getRight(); }
    glm::vec3 getUp() const { return transform.getUp(); }
    
    void resetTransform() { transform.reset(); }

};
