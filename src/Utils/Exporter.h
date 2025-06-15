#pragma once

#include <string>
#include <vector>
#include "../src/Drawables/Mesh.h"  // Your Mesh class path
#include "../src/Generators/TerrainGenerator.h"

// Structure to hold simplified mesh data for exporting
struct MeshData {
    std::string name;
    std::vector<float> vertices;             // Format: x, y, z, u, v
    std::vector<unsigned int> indices;

    MeshData(const std::string& name, const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
        : name(name), vertices(vertices), indices(indices) {}
};

// Global container to hold all meshes to export
extern std::vector<MeshData*> list_of_meshes;

// Converts a Mesh object into MeshData format
MeshData* ConvertMeshToMeshData(const Mesh& mesh, const std::string& name);

// Exports the list of MeshData to an FBX file
void ExportMeshDataAsFBX(MeshData* data, const std::string& filename);
void ExportMeshAsFBX(const Mesh& mesh, const std::string& filename);