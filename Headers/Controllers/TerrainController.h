#ifndef TERRAINCONTROLLER_H
#define TERRAINCONTROLLER_H

#include "UI/TerrainUI.h"
#include "Controllers/GeneratorController.h"
#include "Generators/TerrainGenerator.h"

class TerrainUI;

class TerrainController : public GeneratorController {
public:
    TerrainController(Renderer* renderer);
    ~TerrainController();

    void Update() override;
    void Generate() override;
    void DisplayUI() override;

private:
    TerrainGenerator generator;
    TerrainUI* terrainUI;

};

#endif