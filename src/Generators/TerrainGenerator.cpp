#include "TerrainGenerator.h"
#include <thread>
#include <future>
#include <vector>

Mesh* TerrainGenerator::Generate() {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    const unsigned int numThreads = std::thread::hardware_concurrency();
    std::vector<std::future<ThreadTask>> futures;

    // Divide the terrain into sections for each thread
    unsigned int rowsPerThread = parameters.numCellsLength / numThreads;
    unsigned int remainingRows = parameters.numCellsLength % numThreads;

    unsigned int startI = 0;
    for (unsigned int t = 0; t < numThreads; t++) {
        unsigned int endI = startI + rowsPerThread + (t < remainingRows ? 1 : 0);

        // Launch a thread for each section
        futures.push_back(std::async(std::launch::async, [this, startI, endI]() {
            ThreadTask task;
            task.startI = startI;
            task.endI = endI;
            task.startJ = 0;
            task.endJ = parameters.numCellsWidth;

            for (unsigned int i = task.startI; i < task.endI; i++) {
                for (unsigned int j = task.startJ; j < task.endJ; j++) {
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
                    task.vertices.push_back(vertex);

                    // indices
                    if (i < parameters.numCellsLength - 1 && j < parameters.numCellsWidth - 1) {
                        task.indices.push_back(i * parameters.numCellsWidth + j);
                        task.indices.push_back(i * parameters.numCellsWidth + j + 1);
                        task.indices.push_back((i + 1) * parameters.numCellsWidth + j + 1);

                        task.indices.push_back(i * parameters.numCellsWidth + j);
                        task.indices.push_back((i + 1) * parameters.numCellsWidth + j + 1);
                        task.indices.push_back((i + 1) * parameters.numCellsWidth + j);
                    }
                }
            }

            return task;
        }));

        startI = endI;
    }

    // Collect results from all threads
    for (auto& future : futures) {
        ThreadTask task = future.get();
        vertices.insert(vertices.end(), task.vertices.begin(), task.vertices.end());
        indices.insert(indices.end(), task.indices.begin(), task.indices.end());
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