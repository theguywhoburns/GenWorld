#include <GenWorld/Core/stb_image_write.h>
#include <GenWorld/Utils/Exporter/MeshExporter.h>
#include <GenWorld/Utils/Utils.h>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <filesystem>
#include <iostream>

namespace Utils {

void OBJTerrainExporter::ExportTerrainAsOBJWithDialog(
    const TerrainMesh &terrain, GLFWwindow *window) {
  std::string filename = Utils::FileDialogs::SaveFile(
      "Export Terrain as OBJ",
      "OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0", window);

  if (filename.empty()) {
    std::cout << "Export cancelled by user" << std::endl;
    return;
  }

  filename = ExportUtils::GetValidOBJFilename(filename);
  ExportTerrainAsOBJ(terrain, filename);
}

void OBJTerrainExporter::ExportTerrainAsOBJ(const TerrainMesh &terrain,
                                            const std::string &filename) {
  std::filesystem::path outPath(filename);
  std::string outputDir = outPath.parent_path().string();
  std::string terrainTexturePath;

  aiScene *scene =
      CreateSceneFromTerrain(terrain, terrainTexturePath, outputDir);
  if (!scene) {
    std::cerr << "Failed to create scene for OBJ export" << std::endl;
    return;
  }

  Assimp::Exporter exporter;
  aiReturn result = exporter.Export(scene, "obj", filename, 0);

  if (result == AI_SUCCESS) {
    std::cout << "Exported terrain to OBJ: " << filename << std::endl;
  } else {
    std::cerr << "OBJ export failed: " << exporter.GetErrorString()
              << std::endl;
  }

  delete scene;
}

bool OBJTerrainExporter::ExportTerrainTextureFromGPU(
    const TerrainMesh &terrain, const std::string &outputPath) {
  if (terrain.getTextureID() == 0) {
    std::cerr << "ERROR: Terrain has no baked texture to export!" << std::endl;
    return false;
  }

  const int texSize = 1024; // Could be made configurable
  std::vector<unsigned char> pixels(texSize * texSize * 3); // RGB

  glBindTexture(GL_TEXTURE_2D, terrain.getTextureID());
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
  glBindTexture(GL_TEXTURE_2D, 0);

  if (!stbi_write_png(outputPath.c_str(), texSize, texSize, 3, pixels.data(),
                      texSize * 3)) {
    std::cerr << "Failed to write terrain texture to: " << outputPath
              << std::endl;
    return false;
  }

  std::cout << "Exported terrain texture: " << outputPath << std::endl;
  return true;
}

MeshData *OBJTerrainExporter::ConvertMeshToMeshData(const Mesh &mesh,
                                                    const std::string &name) {
  return ExportUtils::ConvertMeshToMeshData(mesh, name);
}

MeshData *OBJTerrainExporter::ConvertMeshToMeshDataWithTransform(
    const Mesh &mesh, const std::string &name, const glm::mat4 &transform) {
  return ExportUtils::ConvertMeshToMeshDataWithTransform(mesh, name, transform);
}

aiMesh *
OBJTerrainExporter::ConvertMeshDataToAssimp(MeshData *data, aiScene *scene,
                                            const std::string &texturePath,
                                            const std::string &outputDir) {
  return ExportUtils::ConvertMeshDataToAssimp(data, scene, texturePath,
                                              outputDir);
}

aiNode *OBJTerrainExporter::CreateNodeWithMesh(aiScene *scene,
                                               const std::string &nodeName,
                                               aiMesh *mesh) {
  return ExportUtils::CreateNodeWithMesh(scene, nodeName, mesh);
}

void OBJTerrainExporter::AddChildNode(aiNode *parent, aiNode *child) {
  ExportUtils::AddChildNode(parent, child);
}

aiScene *
OBJTerrainExporter::CreateSceneFromTerrain(const TerrainMesh &terrain,
                                           std::string &terrainTexturePath,
                                           const std::string &outputDir) {
  aiScene *scene = new aiScene();
  scene->mRootNode = new aiNode();
  scene->mRootNode->mName = aiString("TerrainScene");
  scene->mMeshes = nullptr;
  scene->mNumMeshes = 0;
  scene->mMaterials = nullptr;
  scene->mNumMaterials = 0;

  // Export terrain texture
  terrainTexturePath = "terrain_color_exported.png";
  if (!ExportTerrainTextureFromGPU(terrain, terrainTexturePath)) {
    std::cerr << "Warning: Failed to export terrain texture" << std::endl;
  }

  // Create terrain mesh
  MeshData *terrainData = ConvertMeshToMeshData(terrain, "Terrain");
  aiMesh *terrainMesh = ConvertMeshDataToAssimp(terrainData, scene,
                                                terrainTexturePath, outputDir);
  aiNode *terrainNode = CreateNodeWithMesh(scene, "Terrain", terrainMesh);
  AddChildNode(scene->mRootNode, terrainNode);
  delete terrainData;

  // Export instance meshes
  const auto &instanceMeshes = terrain.getInstanceMeshes();
  const auto &modelInstances = terrain.getModelInstances();
  int instanceIndex = 0;

  for (const auto &[modelPath, transforms] : modelInstances) {
    auto modelIt = instanceMeshes.find(modelPath);
    if (modelIt == instanceMeshes.end())
      continue;

    const auto &model = modelIt->second;
    const auto &meshes = model->getMeshes();

    for (const auto &transform : transforms) {
      aiNode *groupNode = new aiNode();
      groupNode->mName = aiString("DecoGroup_" + std::to_string(instanceIndex));

      for (size_t i = 0; i < meshes.size(); ++i) {
        Mesh *mesh = meshes[i];
        MeshData *decoData = ConvertMeshToMeshDataWithTransform(
            *mesh,
            "Deco_" + std::to_string(instanceIndex) + "_Mesh_" +
                std::to_string(i),
            transform);
        std::string texture = mesh->getTexturePath();
        aiMesh *decoMesh =
            ConvertMeshDataToAssimp(decoData, scene, texture, outputDir);
        aiNode *decoNode = CreateNodeWithMesh(scene, decoData->name, decoMesh);
        AddChildNode(groupNode, decoNode);
        delete decoData;
      }

      AddChildNode(terrainNode, groupNode);
      instanceIndex++;
    }
  }

  return scene;
}
} // namespace Utils