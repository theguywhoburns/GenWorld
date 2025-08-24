#pragma once

#include <GenWorld/Core/stb_image_write.h>
#include <GenWorld/Drawables/BlockMesh.h>
#include <GenWorld/Drawables/Mesh.h>
#include <GenWorld/Drawables/TerrainMesh.h>
#include <GenWorld/Utils/MeshData.h>
#include <GenWorld/Utils/OpenGlInc.h>
#include <assimp/scene.h>

namespace Utils {
class MeshExporter {
public:
  static void ExportMesh(const Mesh *mesh, const std::string &format,
                         GLFWwindow *window = nullptr);

private:
  static void ExportMeshAsObj(const Mesh *mesh, GLFWwindow *window);
  // Future file extensions can be added here
};

class OBJTerrainExporter {
public:
  static void ExportTerrainAsOBJ(const TerrainMesh &terrain,
                                 const std::string &filename);
  static void ExportTerrainAsOBJWithDialog(const TerrainMesh &terrain,
                                           GLFWwindow *window = nullptr);

private:
  static bool ExportTerrainTextureFromGPU(const TerrainMesh &terrain,
                                          const std::string &outputPath);
  static MeshData *ConvertMeshToMeshData(const Mesh &mesh,
                                         const std::string &name);
  static MeshData *
  ConvertMeshToMeshDataWithTransform(const Mesh &mesh, const std::string &name,
                                     const glm::mat4 &transform);
  static aiMesh *ConvertMeshDataToAssimp(MeshData *data, aiScene *scene,
                                         const std::string &texturePath,
                                         const std::string &outputDir);
  static aiNode *CreateNodeWithMesh(aiScene *scene, const std::string &nodeName,
                                    aiMesh *mesh);
  static aiScene *CreateSceneFromTerrain(const TerrainMesh &terrain,
                                         std::string &terrainTexturePath,
                                         const std::string &outputDir);
  static void AddChildNode(aiNode *parent, aiNode *child);
};

class OBJBlockExporter {
public:
  static void ExportBlockMeshAsOBJ(const BlockMesh &blockMesh,
                                   const std::string &filename);
  static void ExportBlockMeshAsOBJWithDialog(const BlockMesh &blockMesh,
                                             GLFWwindow *window = nullptr);

private:
  static MeshData *ConvertMeshToMeshData(const Mesh &mesh,
                                         const std::string &name);
  static MeshData *
  ConvertMeshToMeshDataWithTransform(const Mesh &mesh, const std::string &name,
                                     const glm::mat4 &transform);
  static aiMesh *ConvertMeshDataToAssimp(MeshData *data, aiScene *scene,
                                         const std::string &texturePath,
                                         const std::string &outputDir);
  static aiNode *CreateNodeWithMesh(aiScene *scene, const std::string &nodeName,
                                    aiMesh *mesh);
  static aiScene *CreateSceneFromBlockMesh(const BlockMesh &blockMesh,
                                           const std::string &outputDir);
  static void AddChildNode(aiNode *parent, aiNode *child);
};

// Common utility functions used by both exporters
namespace ExportUtils {
MeshData *ConvertMeshToMeshData(const Mesh &mesh, const std::string &name);
MeshData *ConvertMeshToMeshDataWithTransform(const Mesh &mesh,
                                             const std::string &name,
                                             const glm::mat4 &transform);
aiMesh *ConvertMeshDataToAssimp(MeshData *data, aiScene *scene,
                                const std::string &texturePath = "",
                                const std::string &outputDir = "");
aiNode *CreateNodeWithMesh(aiScene *scene, const std::string &nodeName,
                           aiMesh *mesh);
void AddChildNode(aiNode *parent, aiNode *child);
std::string GetValidOBJFilename(const std::string &filename);
} // namespace ExportUtils
} // namespace Utils