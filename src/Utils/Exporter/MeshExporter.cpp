#include "MeshExporter.h"

void Utils::MeshExporter::ExportMesh(const Mesh* mesh, std::string format) {
    if (format == "obj") {
        ExportMeshAsObj(mesh, "exported_mesh.obj");
    }

    // more extensions can be added here
    else {
        std::cerr << "Unsupported mesh format: " << format << "\n";
    }
}

void Utils::MeshExporter::ExportMeshAsObj(const Mesh* mesh, const std::string& filename) {

    // Safely cast from Mesh* to TerrainMesh*
    {
        const TerrainMesh* terrainData = dynamic_cast<const TerrainMesh*>(mesh);
        if (terrainData != nullptr) {
            OBJTerrainExporter::ExportTerrainAsOBJ(*terrainData, "terrain.obj");
            return;
        }
    }

    // Check for block generation
    {
        const BlockMesh* blockData = dynamic_cast<const BlockMesh*>(mesh);
        if (blockData != nullptr) {
            // OBJBlockExporter::ExportBlockAsOBJ(*blockData, "block.obj");
            return;
        }
    }

    std::cerr << "Mesh is not a TerrainMesh or BlockMesh.\n";
}
