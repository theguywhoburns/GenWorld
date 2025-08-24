#include "../Utils.h"
#include "MeshExporter.h"
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace Utils::ExportUtils {

MeshData *ConvertMeshToMeshData(const Mesh &mesh, const std::string &name) {
  std::vector<float> vertexData;
  std::vector<unsigned int> indexData = mesh.indices;

  const int totalVertices = mesh.vertices.size();
  vertexData.reserve(totalVertices * 5); // Position (3) + TexCoord (2)

  for (size_t i = 0; i < totalVertices; ++i) {
    const auto &v = mesh.vertices[i];

    // Position
    vertexData.push_back(v.Position.x);
    vertexData.push_back(v.Position.y);
    vertexData.push_back(v.Position.z);

    // TexCoord
    vertexData.push_back(v.TexCoords.x);
    vertexData.push_back(v.TexCoords.y);
  }

  return new MeshData(name, vertexData, indexData);
}

MeshData *ConvertMeshToMeshDataWithTransform(const Mesh &mesh,
                                             const std::string &name,
                                             const glm::mat4 &transform) {
  std::vector<float> vertexData;
  std::vector<unsigned int> indexData = mesh.indices;

  vertexData.reserve(mesh.vertices.size() * 5); // Position (3) + TexCoord (2)

  for (const auto &v : mesh.vertices) {
    glm::vec4 transformedPos = transform * glm::vec4(v.Position, 1.0f);

    vertexData.push_back(transformedPos.x);
    vertexData.push_back(transformedPos.y);
    vertexData.push_back(transformedPos.z);
    vertexData.push_back(v.TexCoords.x);
    vertexData.push_back(v.TexCoords.y);
  }

  return new MeshData(name, vertexData, indexData);
}

aiMesh *ConvertMeshDataToAssimp(MeshData *data, aiScene *scene,
                                const std::string &texturePath,
                                const std::string &outputDir) {
  const int stride = 5;
  const int numVertices = static_cast<int>(data->vertices.size() / stride);
  const int numFaces = static_cast<int>(data->indices.size() / 3);

  if (data->vertices.empty() || data->indices.empty()) {
    std::cerr << "⚠️ Skipping mesh with no geometry: " << data->name
              << std::endl;
    return nullptr;
  }

  aiMesh *mesh = new aiMesh();
  mesh->mName = aiString(data->name);
  mesh->mNumVertices = numVertices;
  mesh->mNumFaces = numFaces;
  mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

  mesh->mVertices = new aiVector3D[numVertices];
  mesh->mTextureCoords[0] = new aiVector3D[numVertices];
  mesh->mNumUVComponents[0] = 2;

  for (int i = 0; i < numVertices; ++i) {
    mesh->mVertices[i] = aiVector3D(data->vertices[i * stride + 0],
                                    data->vertices[i * stride + 1],
                                    data->vertices[i * stride + 2]);

    float u = data->vertices[i * stride + 3];
    float v = 1.0f - data->vertices[i * stride + 4]; // Flip V coordinate
    mesh->mTextureCoords[0][i] = aiVector3D(u, v, 0.0f);
  }

  mesh->mFaces = new aiFace[numFaces];
  for (int i = 0; i < numFaces; ++i) {
    aiFace &face = mesh->mFaces[i];
    face.mNumIndices = 3;
    face.mIndices =
        new unsigned int[3]{data->indices[i * 3 + 0], data->indices[i * 3 + 1],
                            data->indices[i * 3 + 2]};
  }

  // Handle texture if provided
  if (!texturePath.empty()) {
    unsigned int materialIndex = scene->mNumMaterials;

    // Check if material already exists
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
      aiString matTexPath;
      if (scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0,
                                           &matTexPath) == AI_SUCCESS &&
          std::string(matTexPath.C_Str()) ==
              std::filesystem::path(texturePath).filename().string()) {
        materialIndex = i;
        break;
      }
    }

    // Create new material if not found
    if (materialIndex == scene->mNumMaterials) {
      aiMaterial *material = new aiMaterial();

      aiString materialName(data->name + "_Material");
      material->AddProperty(&materialName, AI_MATKEY_NAME);

      aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
      material->AddProperty(&diffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);

      std::string textureFilename =
          std::filesystem::path(texturePath).filename().string();
      aiString texPath(textureFilename);
      material->AddProperty(&texPath,
                            AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

      // Add material to scene
      aiMaterial **newMaterials = new aiMaterial *[scene->mNumMaterials + 1];
      for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
        newMaterials[i] = scene->mMaterials[i];
      newMaterials[scene->mNumMaterials] = material;

      delete[] scene->mMaterials;
      scene->mMaterials = newMaterials;
      scene->mNumMaterials++;

      // Copy texture to output folder
      try {
        if (!outputDir.empty()) {
          std::filesystem::path destPath =
              std::filesystem::path(outputDir) / textureFilename;
          if (!std::filesystem::exists(destPath)) {
            std::filesystem::copy(
                texturePath, destPath,
                std::filesystem::copy_options::overwrite_existing);
            std::cout << "Copied texture to: " << destPath << std::endl;
          }
        }
      } catch (const std::exception &e) {
        std::cerr << "Failed to copy texture: " << e.what() << std::endl;
      }
    }

    mesh->mMaterialIndex = materialIndex;
  }

  return mesh;
}

aiNode *CreateNodeWithMesh(aiScene *scene, const std::string &nodeName,
                           aiMesh *mesh) {
  aiNode *node = new aiNode();
  node->mName = aiString(nodeName);
  node->mNumMeshes = 1;
  node->mMeshes = new unsigned int[1];
  node->mMeshes[0] = scene->mNumMeshes;

  // Add mesh to scene
  aiMesh **newMeshes = new aiMesh *[scene->mNumMeshes + 1];
  for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
    newMeshes[i] = scene->mMeshes[i];
  }
  newMeshes[scene->mNumMeshes] = mesh;

  delete[] scene->mMeshes;
  scene->mMeshes = newMeshes;
  scene->mNumMeshes++;

  return node;
}

void AddChildNode(aiNode *parent, aiNode *child) {
  aiNode **newChildren = new aiNode *[parent->mNumChildren + 1];
  for (unsigned int i = 0; i < parent->mNumChildren; ++i) {
    newChildren[i] = parent->mChildren[i];
  }
  newChildren[parent->mNumChildren] = child;
  child->mParent = parent;

  delete[] parent->mChildren;
  parent->mChildren = newChildren;
  parent->mNumChildren++;
}

std::string GetValidOBJFilename(const std::string &filename) {
  std::string normalizedPath = Utils::NormalizePath(filename);

  std::filesystem::path outPath(normalizedPath);
  if (outPath.extension() != ".obj") {
    outPath.replace_extension(".obj");
    normalizedPath = Utils::NormalizePath(outPath.string());
  }

  return normalizedPath;
}
} // namespace Utils::ExportUtils