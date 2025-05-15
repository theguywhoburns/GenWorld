#ifndef GENERATORSTRATEGY_H
#define GENERATORSTRATEGY_H

#include "../Drawables/Mesh.h"

class IGeneratorStrategy {
public:
    virtual void Generate() = 0;
    virtual Mesh* GetMesh() const = 0;
};

#endif