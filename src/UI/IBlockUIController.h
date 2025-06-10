#pragma once
#include <string>

// Interface to break circular dependency
class IBlockUIController {
public:
    virtual ~IBlockUIController() = default;
    virtual void Generate() = 0;
    virtual void LoadModel(const std::string& filepath) = 0;
};