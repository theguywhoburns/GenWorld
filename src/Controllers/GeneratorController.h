#ifndef GENERATORCONTROLLER_H
#define GENERATORCONTROLLER_H

#include "../Renderers/Renderer.h"
#include "../UI/GeneratorUI.h"
#include "../Generators/IGeneratorStrategy.h"

class GeneratorController {
public:
    GeneratorController(Renderer* renderer) : renderer(renderer) {}
    virtual ~GeneratorController() = default;

    virtual void Generate() = 0;
    virtual void DisplayUI() = 0;
    virtual void Update() = 0;
    virtual void RandomizeSeed() = 0;
    virtual IGeneratorStrategy& getGenerator() = 0;

protected:
    Renderer* renderer = nullptr;

    virtual void UpdateParameters() = 0;

};

#endif