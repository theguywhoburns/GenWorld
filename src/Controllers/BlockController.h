#pragma once
#include "GeneratorController.h"
#include "../UI/IBlockUIController.h"

class BlockGenerator;
class BlockUI;
class Mesh;
class Renderer;

class BlockController : public GeneratorController, public IBlockUIController {
public:
    BlockController(Renderer* renderer);
    ~BlockController() override;

    void DisplayUI() override;
    void Update() override;
    void RandomizeSeed() override;
    IGeneratorStrategy& getGenerator() override;
    BlockUI* GetBlockUI() {
        return blockUI;
    }
    BlockGenerator* GetGenerator() {
        return generator;
    }
    void Generate() override;
    void UpdateParameters();
    void LoadModel(const std::string& filepath) override;

private:
    BlockGenerator* generator;
    BlockUI* blockUI;
    Mesh* blockMesh;
};

