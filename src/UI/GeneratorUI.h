#ifndef GENERATORUI_H
#define GENERATORUI_H

class GeneratorUI {
public:
    virtual void DisplayUI() = 0;
    virtual void RandomizeSeed() = 0;
    virtual ~GeneratorUI() = default;
};

#endif