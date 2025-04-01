#ifndef GENERATORCONTROLLER_H
#define GENERATORCONTROLLER_H

#include "../Renderers/Renderer.h"
#include "../UI/GeneratorUI.h"

class GeneratorController {
public:
    GeneratorController(Renderer* renderer) : renderer(renderer) {}
    virtual ~GeneratorController() { delete renderer; }

    virtual void Generate() = 0;
    virtual void DisplayUI() = 0;
    virtual void Update() = 0;
    
protected:
    Renderer* renderer;

    virtual void UpdateParameters() = 0;

};

#endif