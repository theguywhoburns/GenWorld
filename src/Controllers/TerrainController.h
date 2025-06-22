#ifndef TERRAINCONTROLLER_H
#define TERRAINCONTROLLER_H

#include "../UI/TerrainUI.h"
#include "../Generators/TerrainGenerator.h"
#include "GeneratorController.h"

class TerrainUI;

class TerrainController : public GeneratorController {
public:
    TerrainController(Renderer* renderer);
    ~TerrainController();

    void Generate() override;
    void DisplayUI() override;
    void Update() override;
    TerrainGenerator& getGenerator() {
        return generator;
    }


private:
    TerrainGenerator generator;
    TerrainUI* terrainUI;

    void UpdateParameters() override;

};

#endif