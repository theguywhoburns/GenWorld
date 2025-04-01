#ifndef GENERATORSTRATEGY_H
#define GENERATORSTRATEGY_H

#include "../Drawables/Mesh.h"

class IGeneratorStrategy {
public:
    virtual Mesh* Generate() = 0;
};

#endif