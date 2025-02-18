#include <TerrainGenerator.h>
#include <ImGuiCurveTest.h>

Mesh* TerrainGenerator::GenerateTerrain() {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	
	for (unsigned int i = 0; i < terrainData.numCellsLength; i++) {
		for (unsigned int j = 0; j < terrainData.numCellsWidth; j++) {
			Vertex vertex;
			
			float x = j * terrainData.stepX - terrainData.halfWidth;
			float z = i * terrainData.stepZ - terrainData.halfLength;
			float y = PerlinNoise(x, z);
			y *= ImGui::EvaluateCurve(terrainData.curvePoints, y);

			// Calculate color
			glm::vec3 vertexColor = getColor(y);
			vertex.Normal = vertexColor;			//! change this later to normal


			y *= terrainData.heightMultiplier;
			vertex.Position = glm::vec3(x, y, z);

			// Calculate UVs
			vertex.TexCoords = glm::vec2((float)j / (terrainData.numCellsWidth - 1), (float)i / (terrainData.numCellsLength - 1));
			vertices.push_back(vertex);

			// indices
			if (i < terrainData.numCellsLength - 1 && j < terrainData.numCellsWidth - 1) {
				indices.push_back(i * terrainData.numCellsWidth + j);
				indices.push_back(i * terrainData.numCellsWidth + j + 1);
				indices.push_back((i + 1) * terrainData.numCellsWidth + j + 1);

				indices.push_back(i * terrainData.numCellsWidth + j);
				indices.push_back((i + 1) * terrainData.numCellsWidth + j + 1);
				indices.push_back((i + 1) * terrainData.numCellsWidth + j);
			}
		}
	}

	return new Mesh(vertices, indices, vector<Texture>());
}

void TerrainGenerator::SetTerrainData(float width, float length, int cellSize, float heightMultiplier, ImGui::point curvePoints[4]) {
	terrainData.width = width;
	terrainData.length = length;
	terrainData.cellSize = cellSize;
	terrainData.heightMultiplier = heightMultiplier;
	terrainData.halfWidth = width / 2;
	terrainData.halfLength = length / 2;
	terrainData.curvePoints = curvePoints;

	terrainData.numCellsWidth = static_cast<int>(width / cellSize) + 1;
	terrainData.numCellsLength = static_cast<int>(length / cellSize) + 1;

	if (terrainData.numCellsWidth < 2) terrainData.numCellsWidth = 2;
	if (terrainData.numCellsLength < 2) terrainData.numCellsLength = 2;

	terrainData.stepX = width / (terrainData.numCellsWidth - 1);
	terrainData.stepZ = length / (terrainData.numCellsLength - 1);
}

void TerrainGenerator::SetNoiseParameters(float lacunarity, float persistence, float scale) {
	terrainData.lacunarity = lacunarity;
	terrainData.persistence = persistence;
	terrainData.scale = scale;
}

void TerrainGenerator::SetNoiseSettings(int octaves, int seed, glm::vec2 offset) {
	terrainData.octaves = octaves;
	terrainData.seed = seed;
	terrainData.offset = offset;
	updateSeedOffset();
}

void TerrainGenerator::SetColorData(vector<VertexColor> colors) {
	terrainData.colors = colors;
}

float TerrainGenerator::PerlinNoise(float x, float z) {
	float frequency = 1;
	float amplitude = 1;
	float noiseHeight = 0;

	for (int j = 0; j < terrainData.octaves; j++) {
		float sampleX = (x - terrainData.halfWidth) / terrainData.scale * frequency + terrainData.octaveOffsets[j].x * frequency;
		float sampleZ = (z - terrainData.halfLength) / terrainData.scale * frequency - terrainData.octaveOffsets[j].y * frequency;

		float perlinValue = glm::perlin(glm::vec2(sampleX, sampleZ));
		perlinValue = (perlinValue + 1.0f) * 0.5f;
		noiseHeight += perlinValue * amplitude;

		amplitude *= terrainData.persistence;
		frequency *= terrainData.lacunarity;
	}

	return noiseHeight;
}

void TerrainGenerator::updateSeedOffset() {
	// Seed random number generator
	srand(terrainData.seed);
	terrainData.octaveOffsets.resize(terrainData.octaves);

	for (int i = 0; i < terrainData.octaves; i++) {
		float offsetX = static_cast<float>(rand() % 10000 - 5000) + terrainData.offset.x;
		float offsetY = static_cast<float>(rand() % 10000 - 5000) + terrainData.offset.y;
		terrainData.octaveOffsets[i] = glm::vec2(offsetX, offsetY);
	}
}

glm::vec3 TerrainGenerator::getColor(float height) {
	if(height < terrainData.colors[0].height) {
		return terrainData.colors[0].color;
	}
	
	for (unsigned int i = 0; i < terrainData.colors.size() - 1; i++) {
		if(height < terrainData.colors[i].height) {
			return terrainData.colors[i].color;
		}
	}

	return terrainData.colors[terrainData.colors.size() - 1].color;
}
