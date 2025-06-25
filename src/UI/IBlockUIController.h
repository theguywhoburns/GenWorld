#pragma once
#include <string>
#include <memory>

class Model;

class IBlockUIController {
public:
    virtual ~IBlockUIController() = default;
    
    virtual void Generate() = 0;
    virtual void LoadModel(const std::string& filepath) = 0;
};