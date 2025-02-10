#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#include <Mesh.h>
#include <ImGuiCurveTest.h>

class TerrainGenerator {
private:
	// Terrain Data
	float width;
	float length;
	int cellSize;
	float heightMultiplier;
	unsigned int numCellsWidth;
	unsigned int numCellsLength;
	float stepX;
	float stepZ;
	ImGui::point* curvePoints;

	// Noise Parameters
	float lacunarity = 2.0f;	// Adjust for frequency of noise
	float persistence = 0.5f;	// Adjust for amplitude of noise
	float scale = 2.0f;			// Adjust for overall height of terrain

	// Noise Settings
	int octaves;
	int seed;
	glm::vec2 offset;
	vector<glm::vec2> octaveOffsets;

public:
	struct VertexColor {
		float height;
		glm::vec4 color;
	};
	Mesh* GenerateTerrain();
	void SetTerrainData(float width, float length, int cellSize, float heightMultiplier, ImGui::point v[4]);
	void SetNoiseParameters(float lacunarity, float persistence, float scale);
	void SetNoiseSettings(int octaves, int seed, glm::vec2 offset);
	void SetColorData(VertexColor colors[4]);

private:
	float PerlinNoise(float x, float z);
	void updateSeedOffset();
	glm::vec3 getColor(float height);
	
	// Color Data
	VertexColor colors[4];
	float halfWidth;
	float halfLength;

};
