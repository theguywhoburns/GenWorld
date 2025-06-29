#pragma once

#include "Shader.h"
#include <memory>
#include <unordered_map>

class ShaderManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;

    static ShaderManager* instance;
    ShaderManager() = default;
    ~ShaderManager() = default;

public:
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    static ShaderManager* GetInstance() {
        if (instance == nullptr) {
            instance = new ShaderManager();
        }
        return instance;
    }

    std::shared_ptr<Shader> getShader(const std::string& name) {
        auto it = shaders.find(name);
        if (it != shaders.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::shared_ptr<Shader> loadShader(const std::string& name,
        const char* vertexPath, const char* fragmentPath) {
        auto shader = std::make_shared<Shader>(vertexPath, fragmentPath);
        shaders[name] = shader;
        return shader;
    }
};
