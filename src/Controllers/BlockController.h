#ifndef BLOCKCONTROLLER_H
#define BLOCKCONTROLLER_H

// #include "../UI/BlockUI.h"
#include "../Generators/BlockGenerator.h"
#include "GeneratorController.h"

class BlockUI;

class BlockController : public GeneratorController{
public:
    BlockController(Renderer* renderer);
    ~BlockController();

    void Generate() override;
    void DisplayUI() override;
    void Update() override;

private:
    BlockGenerator generator;
    BlockUI* blockUI;
    Mesh* blockMesh;

    void UpdateParameters() override;


    
};

#endif