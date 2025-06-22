#pragma once

#include <string>
#include <vector>
#include "../src/Drawables/Mesh.h"
#include "../src/Generators/TerrainGenerator.h"
#include "../src/Drawables/TerrainMesh.h"

struct MeshData {
    std::string name;
    std::vector<float> vertices; // x, y, z, u, v, r, g, b
    std::vector<unsigned int> indices;

    MeshData(const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
        : name(name), vertices(vertices), indices(indices) {}
};

MeshData* ConvertMeshToMeshData(const Mesh& mesh, const std::string& name);

void ExportMeshDataAsFBX(MeshData* data, const std::string& filename);
void ExportMeshAsFBX(const Mesh& mesh, const std::string& filename);

// NEW: TerrainMesh support
void ExportMeshAsFBX(const TerrainMesh& terrain, const std::string& filename);
