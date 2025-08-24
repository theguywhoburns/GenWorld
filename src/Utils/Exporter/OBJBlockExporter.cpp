#include "../Utils.h"
#include "MeshExporter.h"
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <filesystem>
#include <iostream>

namespace Utils {

void OBJBlockExporter::ExportBlockMeshAsOBJWithDialog(
    const BlockMesh &blockMesh, GLFWwindow *window) {
  std::string filename = Utils::FileDialogs::SaveFile(
      "Export Block Mesh as OBJ",
      "OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0", window);

  if (filename.empty()) {
    std::cout << "Export cancelled by user" << std::endl;
    return;
  }

  filename = ExportUtils::GetValidOBJFilename(filename);
  ExportBlockMeshAsOBJ(blockMesh, filename);
}

void OBJBlockExporter::ExportBlockMeshAsOBJ(const BlockMesh &blockMesh,
                                            const std::string &filename) {
  std::filesystem::path outPath(filename);
  std::string outputDir = outPath.parent_path().string();

  aiScene *scene = CreateSceneFromBlockMesh(blockMesh, outputDir);
  if (!scene) {
    std::cerr << "Failed to create scene for OBJ export" << std::endl;
    return;
  }

  Assimp::Exporter exporter;
  aiReturn result = exporter.Export(scene, "obj", filename, 0);

  if (result == AI_SUCCESS) {
    std::cout << "Exported block mesh to OBJ: " << filename << std::endl;
  } else {
    std::cerr << "OBJ export failed: " << exporter.GetErrorString()
              << std::endl;
  }

  delete scene;
}

MeshData *OBJBlockExporter::ConvertMeshToMeshData(const Mesh &mesh,
                                                  const std::string &name) {
  return ExportUtils::ConvertMeshToMeshData(mesh, name);
}

MeshData *OBJBlockExporter::ConvertMeshToMeshDataWithTransform(
    const Mesh &mesh, const std::string &name, const glm::mat4 &transform) {
  return ExportUtils::ConvertMeshToMeshDataWithTransform(mesh, name, transform);
}

aiMesh *
OBJBlockExporter::ConvertMeshDataToAssimp(MeshData *data, aiScene *scene,
                                          const std::string &texturePath,
                                          const std::string &outputDir) {
  return ExportUtils::ConvertMeshDataToAssimp(data, scene, texturePath,
                                              outputDir);
}

aiNode *OBJBlockExporter::CreateNodeWithMesh(aiScene *scene,
                                             const std::string &nodeName,
                                             aiMesh *mesh) {
  return ExportUtils::CreateNodeWithMesh(scene, nodeName, mesh);
}

void OBJBlockExporter::AddChildNode(aiNode *parent, aiNode *child) {
  ExportUtils::AddChildNode(parent, child);
}

aiScene *
OBJBlockExporter::CreateSceneFromBlockMesh(const BlockMesh &blockMesh,
                                           const std::string &outputDir) {
  aiScene *scene = new aiScene();
  scene->mRootNode = new aiNode();
  scene->mRootNode->mName = aiString("BlockMeshScene");
  scene->mMeshes = nullptr;
  scene->mNumMeshes = 0;
  scene->mMaterials = nullptr;
  scene->mNumMaterials = 0;

  // Export the root BlockMesh base geometry (if any)
  std::string baseTexture = blockMesh.getTexturePath();
  MeshData *meshData = ConvertMeshToMeshData(blockMesh, "BlockMesh");

  // Skip if no geometry
  if (!meshData || meshData->vertices.empty() || meshData->indices.empty()) {
    std::cerr << "Block mesh is empty. Skipping base mesh export.\n";
    delete meshData;
  } else {
    aiMesh *mesh =
        ConvertMeshDataToAssimp(meshData, scene, baseTexture, outputDir);
    if (mesh) {
      aiNode *meshNode = CreateNodeWithMesh(scene, "BlockMesh", mesh);
      AddChildNode(scene->mRootNode, meshNode);
    }
    delete meshData;
  }

  // Export instance meshes (blockInstances that use asset models)
  const auto &instanceMap = blockMesh.getAssetInstances();
  const auto &modelMap = blockMesh.getAssetModels();

  int instanceIndex = 0;
  for (const auto &[modelPath, transforms] : instanceMap) {
    auto modelIt = modelMap.find(modelPath);
    if (modelIt == modelMap.end())
      continue;

    const auto &model = modelIt->second;
    const auto &meshes = model->getMeshes();

    for (const auto &transform : transforms) {
      aiNode *groupNode = new aiNode();
      groupNode->mName =
          aiString("BlockGroup_" + std::to_string(instanceIndex));

      for (size_t i = 0; i < meshes.size(); ++i) {
        Mesh *subMesh = meshes[i];
        if (!subMesh)
          continue;

        std::string meshName = "BlockInstance_" +
                               std::to_string(instanceIndex) + "_Mesh_" +
                               std::to_string(i);
        MeshData *instanceData =
            ConvertMeshToMeshDataWithTransform(*subMesh, meshName, transform);

        //  Skip if no geometry
        if (!instanceData || instanceData->vertices.empty() ||
            instanceData->indices.empty()) {
          std::cerr << " Skipping empty instance mesh: " << meshName
                    << std::endl;
          delete instanceData;
          continue;
        }

        std::string texture = subMesh->getTexturePath();
        aiMesh *instanceMesh =
            ConvertMeshDataToAssimp(instanceData, scene, texture, outputDir);
        if (!instanceMesh) {
          delete instanceData;
          continue;
        }

        aiNode *node = CreateNodeWithMesh(scene, meshName, instanceMesh);
        AddChildNode(groupNode, node);
        delete instanceData;
      }

      AddChildNode(scene->mRootNode, groupNode);
      ++instanceIndex;
    }
  }

  // Optional: Abort export if nothing was added
  if (scene->mNumMeshes == 0) {
    std::cerr << " No valid geometry found in block mesh. Aborting export.\n";
    delete scene;
    return nullptr;
  }

  return scene;
}

} // namespace Utils
