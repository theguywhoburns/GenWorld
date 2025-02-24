#include "Generators/TerrainGenerator.h"

Mesh* TerrainGenerator::Generate() {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	for (unsigned int i = 0; i < parameters.numCellsLength; i++) {
		for (unsigned int j = 0; j < parameters.numCellsWidth; j++) {
			Vertex vertex;

			float x = j * parameters.stepX - parameters.halfWidth;
			float z = i * parameters.stepZ - parameters.halfLength;
			float y = PerlinNoise(x, z);
			y *= ImGui::EvaluateCurve(parameters.curvePoints, y);

			// Calculate color
			glm::vec3 vertexColor = getColor(y);
			vertex.Normal = vertexColor; //! change this later to normal

			y *= parameters.heightMultiplier;
			vertex.Position = glm::vec3(x, y, z);

			// Calculate UVs
			vertex.TexCoords = glm::vec2((float)j / (parameters.numCellsWidth - 1), (float)i / (parameters.numCellsLength - 1));
			vertices.push_back(vertex);

			// indices
			if (i < parameters.numCellsLength - 1 && j < parameters.numCellsWidth - 1) {
				indices.push_back(i * parameters.numCellsWidth + j);
				indices.push_back(i * parameters.numCellsWidth + j + 1);
				indices.push_back((i + 1) * parameters.numCellsWidth + j + 1);

				indices.push_back(i * parameters.numCellsWidth + j);
				indices.push_back((i + 1) * parameters.numCellsWidth + j + 1);
				indices.push_back((i + 1) * parameters.numCellsWidth + j);
			}
		}
	}

	return new Mesh(vertices, indices, std::vector<Texture>());
}

void TerrainGenerator::SetParameters(const TerrainUtilities::TerrainData& params) {
	parameters = params;

	// Calculate derived values
	parameters.halfWidth = parameters.width / 2;
	parameters.halfLength = parameters.length / 2;
	parameters.numCellsWidth = static_cast<int>(parameters.width / parameters.cellSize) + 1;
	parameters.numCellsLength = static_cast<int>(parameters.length / parameters.cellSize) + 1;

	if (parameters.numCellsWidth < 2) parameters.numCellsWidth = 2;
	if (parameters.numCellsLength < 2) parameters.numCellsLength = 2;

	parameters.stepX = parameters.width / (parameters.numCellsWidth - 1);
	parameters.stepZ = parameters.length / (parameters.numCellsLength - 1);

	// Update noise offsets based on seed
	updateSeedOffset();
}

float TerrainGenerator::PerlinNoise(float x, float z) {
	float frequency = 1;
	float amplitude = 1;
	float noiseHeight = 0;

	for (int j = 0; j < parameters.octaves; j++) {
		float sampleX = (x - parameters.halfWidth) / parameters.scale * frequency + parameters.octaveOffsets[j].x * frequency;
		float sampleZ = (z - parameters.halfLength) / parameters.scale * frequency - parameters.octaveOffsets[j].y * frequency;

		float perlinValue = glm::perlin(glm::vec2(sampleX, sampleZ));
		perlinValue = (perlinValue + 1.0f) * 0.5f;
		noiseHeight += perlinValue * amplitude;

		amplitude *= parameters.persistence;
		frequency *= parameters.lacunarity;
	}

	return noiseHeight;
}

void TerrainGenerator::updateSeedOffset() {
	// Seed random number generator
	srand(parameters.seed);
	parameters.octaveOffsets.resize(parameters.octaves);

	for (int i = 0; i < parameters.octaves; i++) {
		float offsetX = static_cast<float>(rand() % 10000 - 5000) + parameters.offset.x;
		float offsetY = static_cast<float>(rand() % 10000 - 5000) + parameters.offset.y;
		parameters.octaveOffsets[i] = glm::vec2(offsetX, offsetY);
	}
}

glm::vec3 TerrainGenerator::getColor(float height) {
	if (height < parameters.colors[0].height) {
		return parameters.colors[0].color;
	}

	for (unsigned int i = 0; i < parameters.colors.size() - 1; i++) {
		if (height < parameters.colors[i].height) {
			return parameters.colors[i].color;
		}
	}

	return parameters.colors[parameters.colors.size() - 1].color;
}