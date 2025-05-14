#pragma once

#include "../Core/Shader.h"
#include "../Core/ShaderManager.h"
#include <memory>

class IDrawable {
protected:
    std::shared_ptr<Shader> m_shader;
    std::string m_solidShader;
    std::string m_unShadedShader;

public:
    IDrawable() {
        m_solidShader = "solid";
        m_unShadedShader = "unshaded";
        m_shader = ShaderManager::GetInstance()->getShader(m_solidShader);
    }
    
    virtual void Draw(Shader& shader) = 0;
    virtual void Draw(const glm::mat4& view, const glm::mat4& projection) = 0;
    virtual void SetShader(std::shared_ptr<Shader> shader) { m_shader = shader; }
    virtual std::shared_ptr<Shader> GetShader() { return m_shader; }

};
