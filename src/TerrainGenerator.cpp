#include <TerrainGenerator.h>

Mesh* TerrainGenerator::GenerateTerrain() {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	for (unsigned int i = 0; i < numCellsLength; i++) {
		for (unsigned int j = 0; j < numCellsWidth; j++) {
			Vertex vertex;
			
			float x = j * stepX - halfWidth;
			float z = i * stepZ - halfLength;
			float y = PerlinNoise(x, z) * this->heightMultiplier;
			vertex.Position = glm::vec3(x, y, z);


			// Calculate color
			glm::vec3 vertexColor = getColor(y / this->heightMultiplier);
			vertex.Normal = vertexColor;			//! change this later to normal

			// Calculate UVs
			vertex.TexCoords = glm::vec2((float)j / (numCellsWidth - 1), (float)i / (numCellsLength - 1));

			vertices.push_back(vertex);

			// indices
			if (i < numCellsLength - 1 && j < numCellsWidth - 1) {
				indices.push_back(i * numCellsWidth + j);
				indices.push_back(i * numCellsWidth + j + 1);
				indices.push_back((i + 1) * numCellsWidth + j + 1);

				indices.push_back(i * numCellsWidth + j);
				indices.push_back((i + 1) * numCellsWidth + j + 1);
				indices.push_back((i + 1) * numCellsWidth + j);
			}
		}
	}

	return new Mesh(vertices, indices, vector<Texture>());
}

void TerrainGenerator::SetTerrainData(float width, float length, int cellSize, float heightMultiplier) {
	this->width = width;
	this->length = length;
	this->cellSize = cellSize;
	this->heightMultiplier = heightMultiplier;
	this->halfWidth = width / 2;
	this->halfLength = length / 2;

	numCellsWidth = static_cast<int>(width / cellSize) + 1;
	numCellsLength = static_cast<int>(length / cellSize) + 1;

	if (numCellsWidth < 2) numCellsWidth = 2;
	if (numCellsLength < 2) numCellsLength = 2;

	stepX = width / (numCellsWidth - 1);
	stepZ = length / (numCellsLength - 1);
}

void TerrainGenerator::SetNoiseParameters(float lacunarity, float persistence, float scale) {
	this->lacunarity = lacunarity;
	this->persistence = persistence;
	this->scale = scale;
}

void TerrainGenerator::SetNoiseSettings(int octaves, int seed, glm::vec2 offset) {
	this->octaves = octaves;
	this->seed = seed;
	this->offset = offset;
	updateSeedOffset();
}

void TerrainGenerator::SetColorData(VertexColor colors[4]) {
	for (int i = 0; i < 4; i++) {
		this->colors[i] = colors[i];
	}
}

float TerrainGenerator::PerlinNoise(float x, float z) {
	float frequency = 1;
	float amplitude = 1;
	float noiseHeight = 0;

	for (int j = 0; j < octaves; j++) {
		float sampleX = x / (scale * frequency) + octaveOffsets[j].x;
		float sampleZ = z / (scale * frequency) + octaveOffsets[j].y;

		float perlinValue = glm::perlin(glm::vec2(sampleX, sampleZ));
		noiseHeight += perlinValue * amplitude;

		amplitude *= persistence;
		frequency *= lacunarity;
	}

	return noiseHeight;
}

void TerrainGenerator::updateSeedOffset() {
	// Seed random number generator
	srand(seed);
	octaveOffsets.resize(octaves);

	for (int i = 0; i < octaves; i++) {
		float offsetX = static_cast<float>(rand() % 10000 - 5000) + offset.x;
		float offsetY = static_cast<float>(rand() % 10000 - 5000) + offset.y;
		octaveOffsets[i] = glm::vec2(offsetX, offsetY);
	}
}

glm::vec3 TerrainGenerator::getColor(float height) {
	glm::vec3 color = glm::vec3(0.0f);
	if (height <= colors[0].height) return colors[0].color;
	else if (height <= colors[1].height) return colors[1].color;
	else if (height <= colors[2].height) return colors[2].color;
	else return colors[3].color;
}
