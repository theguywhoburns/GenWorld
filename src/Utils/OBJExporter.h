#pragma once

#include "MeshData.h"
#include <assimp/scene.h>

bool ExportTerrainTextureFromGPU(const TerrainMesh& terrain, const std::string& outputPath);

MeshData* ConvertMeshToMeshData(const Mesh& mesh, const std::string& name);
MeshData* ConvertMeshToMeshDataWithTransform(const Mesh& mesh, const std::string& name, const glm::mat4& transform);
aiMesh* ConvertMeshDataToAssimp(MeshData* data, aiScene* scene, const std::string& texturePath, const std::string& outputDir);
aiNode* CreateNodeWithMesh(aiScene* scene, const std::string& nodeName, aiMesh* mesh);
void AddChildNode(aiNode* parent, aiNode* child);

void ExportMeshAsOBJ(const TerrainMesh& terrain, const std::string& filename);