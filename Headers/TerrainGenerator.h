#pragma once

#include <TerrainData.h>
using namespace TerrainUtilities;

class TerrainGenerator {
private:
	TerrainData terrainData;

public:
	Mesh* GenerateTerrain();
	void SetTerrainData(float width, float length, int cellSize, float heightMultiplier, ImGui::point v[4]);
	void SetNoiseParameters(float lacunarity, float persistence, float scale);
	void SetNoiseSettings(int octaves, int seed, glm::vec2 offset);
	void SetColorData(vector<VertexColor> colors);

private:
	float PerlinNoise(float x, float z);
	void updateSeedOffset();
	glm::vec3 getColor(float height);

};
