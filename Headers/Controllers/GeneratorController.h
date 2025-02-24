#ifndef GENERATORCONTROLLER_H
#define GENERATORCONTROLLER_H

#include "Renderer.h"
#include "UI/GeneratorUI.h"

class GeneratorController {
public:
    GeneratorController(Renderer* renderer) : renderer(renderer) {}
    virtual ~GeneratorController() { delete generatorUI; delete renderer; }

    virtual void Update() = 0;
    virtual void Generate() = 0;
    virtual void DisplayUI() = 0;

protected:
    Renderer* renderer;
    GeneratorUI* generatorUI;
};

#endif