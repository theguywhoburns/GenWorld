#include "TerrainGenerator.h"
#include <thread>
#include <future>
#include <vector>
#include <algorithm>
#include "../Utils/perlin.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../Core/stb_image_write.h"

Mesh* TerrainGenerator::Generate() {
    // Generate height map
    heightMap = GenerateHeightMap();

    // Generate mesh from height map
    if (terrainMesh != nullptr)
        delete terrainMesh;

    terrainMesh = GenerateFromHeightMap(heightMap);

    return terrainMesh;
}

std::vector<float> TerrainGenerator::GenerateHeightMap() {
    std::vector<float> heightMap(parameters.numCellsWidth * parameters.numCellsLength);

    const unsigned int numThreads = std::thread::hardware_concurrency();
    std::vector<std::future<void>> futures;

    unsigned int rowsPerThread = parameters.numCellsLength / numThreads;
    unsigned int remainingRows = parameters.numCellsLength % numThreads;

    unsigned int startI = 0;
    for (unsigned int t = 0; t < numThreads; t++) {
        unsigned int endI = startI + rowsPerThread + (t < remainingRows ? 1 : 0);

        futures.push_back(std::async(std::launch::async, [this, &heightMap, startI, endI]() {
            for (unsigned int i = startI; i < endI; i++) {
                for (unsigned int j = 0; j < parameters.numCellsWidth; j++) {
                    float x = j * parameters.stepX - parameters.halfWidth;
                    float z = i * parameters.stepZ - parameters.halfLength;

                    // Calculate height using Perlin noise
                    float y = PerlinNoise(x, z);

                    // Apply height curve
                    y *= ImGui::EvaluateCurve(parameters.curvePoints, y);

                    // Apply falloff map
                    float normalizedX = x / parameters.halfWidth;
                    float normalizedZ = z / parameters.halfLength;
                    float falloffValue = TerrainUtilities::GenerateFalloffValue(normalizedX, normalizedZ, parameters.falloffParams);
                    y *= falloffValue;

                    heightMap[i * parameters.numCellsWidth + j] = y;
                }
            }
            }
        ));

        startI = endI;
    }

    for (auto& future : futures) {
        future.wait();
    }

    // write height map to png
    std::string filename = "heightmap.png";
    SaveHeightMapToPNG(heightMap, filename, parameters.numCellsWidth, parameters.numCellsLength);


    return heightMap;
}

void TerrainGenerator::SaveHeightMapToPNG(const std::vector<float>& heightMap, const std::string& filename, int width, int height) {
    if (heightMap.size() != width * height) {
        std::cerr << "Heightmap size mismatch.\n";
        return;
    }

    // Find min and max height values
    float minVal = *std::min_element(heightMap.begin(), heightMap.end());
    float maxVal = *std::max_element(heightMap.begin(), heightMap.end());
    float range = maxVal - minVal;
    if (range == 0.0f) range = 1.0f; // prevent division by zero

    // Convert to 8-bit grayscale
    std::vector<unsigned char> imageData(width * height);
    for (int i = 0; i < width * height; ++i) {
        float normalized = (heightMap[i] - minVal) / range; // 0 to 1
        imageData[i] = static_cast<unsigned char>(normalized * 255.0f);
    }

    // Write to PNG
    if (!stbi_write_png(filename.c_str(), width, height, 1, imageData.data(), width)) {
        std::cerr << "Failed to write PNG file: " << filename << std::endl;
    }
}

Mesh* TerrainGenerator::GenerateFromHeightMap(const std::vector<float>& heightMap) {
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

        futures.push_back(std::async(std::launch::async, [this, &heightMap, startI, endI]() {
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

                    // Get height from the height map
                    float y = heightMap[i * parameters.numCellsWidth + j];

                    // Calculate color based on height
                    glm::vec3 vertexColor = getColor(y);
                    vertex.Color = vertexColor;

                    // Now apply the height multiplier
                    y *= parameters.heightMultiplier;
                    vertex.Position = glm::vec3(x, y, z);

                    // Calculate UVs
                    vertex.TexCoords = glm::vec2(
                        (float)i / (parameters.numCellsLength - 1),
                        (float)j / (parameters.numCellsWidth - 1)
                    );

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
            }
        ));

        startI = endI;
    }

    // Collect results from all threads
    for (auto& future : futures) {
        ThreadTask task = future.get();
        vertices.insert(vertices.end(), task.vertices.begin(), task.vertices.end());
        indices.insert(indices.end(), task.indices.begin(), task.indices.end());
    }

    CalculateNormals(vertices, indices);

    return new TerrainMesh(vertices, indices, parameters, heightMap);
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
    vector<int> permutation = PerlinNoise::get_permutation_vector();

    for (int j = 0; j < parameters.octaves; j++) {
        float sampleX = (x - parameters.halfWidth) / parameters.scale * frequency + parameters.octaveOffsets[j].x * frequency;
        float sampleZ = (z - parameters.halfLength) / parameters.scale * frequency - parameters.octaveOffsets[j].y * frequency;

        float perlinValue = PerlinNoise::perlin_noise(sampleX, sampleZ, permutation);
        noiseHeight += perlinValue * amplitude;

        // Lacunarity  --> Increase in frequency of octaves
        // Persistence --> Decrease in amplitude of octaves
        amplitude *= parameters.persistence;
        frequency *= parameters.lacunarity;
    }

    return (noiseHeight + 1) / parameters.maxHeight;
}

void TerrainGenerator::updateSeedOffset() {
    // Seed random number generator
    srand(parameters.seed);
    parameters.octaveOffsets.resize(parameters.octaves);

    float amplitude = 1;
    float maxPossibleHeight = 0;

    for (int i = 0; i < parameters.octaves; i++) {
        float offsetX = static_cast<float>(rand() % 10000 - 5000) + parameters.offset.x;
        float offsetY = static_cast<float>(rand() % 10000 - 5000) + parameters.offset.y;
        parameters.octaveOffsets[i] = glm::vec2(offsetX, offsetY);

        maxPossibleHeight += amplitude;
        amplitude *= parameters.persistence;
    }

    parameters.maxHeight = maxPossibleHeight;
}

glm::vec3 TerrainGenerator::getColor(float height) {
    if (parameters.colors.empty())
        return glm::vec3(1.0f);

    if (height <= parameters.colors[0].height)
        return parameters.colors[0].color;

    for (unsigned int i = 0; i < parameters.colors.size() - 1; i++) {
        if (height < parameters.colors[i].height) {
            return parameters.colors[i].color;
        }
    }

    return parameters.colors.back().color;
}

void TerrainGenerator::CalculateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    for (unsigned int i = 0; i < indices.size(); i += 3) {
        unsigned int index0 = indices[i];
        unsigned int index1 = indices[i + 1];
        unsigned int index2 = indices[i + 2];

        glm::vec3 v0 = vertices[index0].Position;
        glm::vec3 v1 = vertices[index1].Position;
        glm::vec3 v2 = vertices[index2].Position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        vertices[index0].Normal += normal;
        vertices[index1].Normal += normal;
        vertices[index2].Normal += normal;
    }

    for (auto& vertex : vertices) {
        vertex.Normal = glm::normalize(vertex.Normal);
    }
}
