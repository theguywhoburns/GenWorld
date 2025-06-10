#pragma once

class Mesh;

class IGeneratorStrategy {
public:
    virtual ~IGeneratorStrategy() = default;
    virtual void Generate() = 0;
    virtual Mesh* GetMesh() const = 0;
};