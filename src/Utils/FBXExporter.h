#pragma once

#include "MeshData.h"
#include <fbxsdk.h>

bool ExportTerrainTextureFromGPUFBX(const TerrainMesh& terrain, const std::string& outputPath);

MeshData* ConvertMeshToMeshDataFBX(const Mesh& mesh, const std::string& name);
MeshData* ConvertMeshToMeshDataWithTransformFBX(const Mesh& mesh, const std::string& name, const glm::mat4& transform);
FbxNode* ConvertMeshDataToFbx(FbxScene* scene, MeshData* data, const std::string& texturePath);

void ExportMeshAsFBX(const TerrainMesh& terrain, const std::string& filename);
