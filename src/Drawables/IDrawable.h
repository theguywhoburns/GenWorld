#pragma once

#include "../Core/Shader.h"
#include "../Core/ShaderManager.h"
#include "../Core/Engine/Transform.h"
#include "../UI/ViewportShading.h"
#include <memory>

class IDrawable {
protected:
    std::shared_ptr<Shader> m_shader;
    std::string m_solidShader;
    std::string m_renderedShader;
    std::string m_wireframeShader;

    Transform transform;
    ShadingParameters m_currentShadingParams;

public:
    IDrawable() {
        m_wireframeShader = "wireframe";
        m_solidShader = "solid";
        m_renderedShader = "rendered";
        SetShader(m_solidShader);

        transform = Transform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    }
    virtual ~IDrawable() = default;

    virtual void Draw(Shader& shader) = 0;
    virtual void Draw(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void SetShader(std::shared_ptr<Shader> shader) { m_shader = shader; }
    virtual void SetShader(const std::string& shaderName) {
        m_shader = ShaderManager::GetInstance()->getShader(shaderName);
        if (!m_shader) {
            throw std::runtime_error("Shader not found: " + shaderName);
        }
    }
    virtual std::shared_ptr<Shader> GetShader() { return m_shader; }

    virtual void setPosition(const glm::vec3& position) { transform.setPosition(position); }
    virtual void setPosition(float x, float y, float z) { transform.setPosition(x, y, z); }
    virtual void setRotation(const glm::vec3& rotation) { transform.setRotation(rotation); }
    virtual void setRotation(float rx, float ry, float rz) { transform.setRotation(rx, ry, rz); }
    virtual void setScale(const glm::vec3& scale) { transform.setScale(scale); }
    virtual void setScale(float uniformScale) { transform.setScale(uniformScale); }
    virtual void setScale(float sx, float sy, float sz) { transform.setScale(sx, sy, sz); }
    virtual void setTransform(const Transform& newTransform) { transform = newTransform; }
    virtual void setTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale) {
        transform.setPosition(position);
        transform.setRotation(rotation);
        transform.setScale(scale);
    }

    virtual const glm::vec3& getPosition() const { return transform.getPosition(); }
    virtual const glm::vec3& getRotation() const { return transform.getRotation(); }
    virtual const glm::vec3& getScale() const { return transform.getScale(); }
    virtual const Transform& getTransform() const { return transform; }
    virtual const glm::mat4& getModelMatrix() const { return transform.getModelMatrix(); }

    void translate(const glm::vec3& translation) { transform.translate(translation); }
    void rotate(const glm::vec3& rotation) { transform.rotate(rotation); }
    void scaleBy(const glm::vec3& scale) { transform.scaleBy(scale); }
    void scaleBy(float uniformScale) { transform.scaleBy(uniformScale); }

    virtual void SetShaderParameters(const ShadingParameters& params) {
        m_currentShadingParams = params;
        switch (m_currentShadingParams.mode) {
        case ViewportShadingMode::Wireframe:
            SetShader(m_wireframeShader);
            break;
        case ViewportShadingMode::SolidColor:
            SetShader(m_solidShader);
            break;
        case ViewportShadingMode::RenderedNoLights:
        case ViewportShadingMode::RenderedWithLights:
            SetShader(m_renderedShader);
            break;
        }
    }

    void resetTransform() { transform.reset(); }

};
