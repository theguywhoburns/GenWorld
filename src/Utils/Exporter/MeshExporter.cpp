#include <GenWorld/Utils/Exporter/MeshExporter.h>
#include <iostream>

void Utils::MeshExporter::ExportMesh(const Mesh *mesh,
                                     const std::string &format,
                                     GLFWwindow *window) {
  if (format == "obj") {
    ExportMeshAsObj(mesh, window);
  } else {
    std::cerr << "Unsupported mesh format: " << format << "\n";
  }
}

void Utils::MeshExporter::ExportMeshAsObj(const Mesh *mesh,
                                          GLFWwindow *window) {
  // Safely cast to TerrainMesh
  {
    const TerrainMesh *terrainData = dynamic_cast<const TerrainMesh *>(mesh);
    if (terrainData != nullptr) {
      OBJTerrainExporter::ExportTerrainAsOBJWithDialog(*terrainData, window);
      return;
    }
  }

  // Check for BlockMesh
  {
    const BlockMesh *blockData = dynamic_cast<const BlockMesh *>(mesh);
    if (blockData != nullptr) {
      OBJBlockExporter::ExportBlockMeshAsOBJWithDialog(*blockData, window);
      return;
    }
  }

  std::cerr << "Mesh is not a TerrainMesh or BlockMesh.\n";
}