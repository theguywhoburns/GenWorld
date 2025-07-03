#include "OBJExporter.h"
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <../src/Core/stb_image_write.h>
#include <filesystem>
#include <string>

// Extracts the resultTextureID from the TerrainMesh and saves it as a PNG
bool ExportTerrainTextureFromGPU(const TerrainMesh& terrain, const std::string& outputPath)
{
    if (terrain.getTextureID() == 0) {
        std::cerr << "ERROR: Terrain has no baked texture to export!" << std::endl;
        return false;
    }

    const int texSize = 1024; // Or query actual size if dynamic

    std::vector<unsigned char> pixels(texSize * texSize * 3); // RGB

    glBindTexture(GL_TEXTURE_2D, terrain.getTextureID());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!stbi_write_png(outputPath.c_str(), texSize, texSize, 3, pixels.data(), texSize * 3)) {
        std::cerr << "Failed to write terrain texture to: " << outputPath << std::endl;
        return false;
    }

    std::cout << "Exported terrain texture: " << outputPath << std::endl;
    return true;
}

MeshData* ConvertMeshToMeshData(const Mesh& mesh, const std::string& name) {
    std::vector<float> vertexData;
    std::vector<unsigned int> indexData = mesh.indices;

    const int totalVertices = mesh.vertices.size();

    for (size_t i = 0; i < totalVertices; ++i) {
        const auto& v = mesh.vertices[i];

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

MeshData* ConvertMeshToMeshDataWithTransform(const Mesh& mesh, const std::string& name, const glm::mat4& transform) {
    std::vector<float> vertexData;
    std::vector<unsigned int> indexData = mesh.indices;

    for (const auto& v : mesh.vertices) {
        glm::vec4 transformedPos = transform * glm::vec4(v.Position, 1.0f);

        vertexData.push_back(transformedPos.x);
        vertexData.push_back(transformedPos.y);
        vertexData.push_back(transformedPos.z);
        vertexData.push_back(v.TexCoords.x);
        vertexData.push_back(v.TexCoords.y);
    }

    return new MeshData(name, vertexData, indexData);
}

aiMesh* ConvertMeshDataToAssimp(MeshData* data, aiScene* scene, const std::string& texturePath = "", const std::string& outputDir = "") {
    const int stride = 5;
    const int numVertices = static_cast<int>(data->vertices.size() / stride);
    const int numFaces = static_cast<int>(data->indices.size() / 3);
    const float exportScale = 100.0f;

    aiMesh* mesh = new aiMesh();
    mesh->mName = aiString(data->name);
    mesh->mNumVertices = numVertices;
    mesh->mNumFaces = numFaces;
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

    mesh->mVertices = new aiVector3D[numVertices];
    mesh->mTextureCoords[0] = new aiVector3D[numVertices];
    mesh->mNumUVComponents[0] = 2;

    for (int i = 0; i < numVertices; ++i) {
        mesh->mVertices[i] = aiVector3D(
            data->vertices[i * stride + 0] * exportScale,
            data->vertices[i * stride + 1] * exportScale,
            data->vertices[i * stride + 2] * exportScale);

        float u = data->vertices[i * stride + 3];
        float v = 1.0f - data->vertices[i * stride + 4];
        mesh->mTextureCoords[0][i] = aiVector3D(u, v, 0.0f);
    }

    mesh->mFaces = new aiFace[numFaces];
    for (int i = 0; i < numFaces; ++i) {
        aiFace& face = mesh->mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3]{
            data->indices[i * 3 + 0],
            data->indices[i * 3 + 1],
            data->indices[i * 3 + 2]
        };
    }

    if (!texturePath.empty()) {
        unsigned int materialIndex = scene->mNumMaterials;

        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiString matTexPath;
            if (scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &matTexPath) == AI_SUCCESS &&
                std::string(matTexPath.C_Str()) == std::filesystem::path(texturePath).filename().string()) {
                materialIndex = i;
                break;
            }
        }

        if (materialIndex == scene->mNumMaterials) {
            aiMaterial* material = new aiMaterial();

            aiString materialName(data->name + "_Material");
            material->AddProperty(&materialName, AI_MATKEY_NAME);

            aiColor3D diffuseColor(1.0f, 1.0f, 1.0f);
            material->AddProperty(&diffuseColor, 1, AI_MATKEY_COLOR_DIFFUSE);

            std::string textureFilename = std::filesystem::path(texturePath).filename().string();
            aiString texPath(textureFilename);
            material->AddProperty(&texPath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

            aiMaterial** newMaterials = new aiMaterial * [scene->mNumMaterials + 1];
            for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
                newMaterials[i] = scene->mMaterials[i];
            newMaterials[scene->mNumMaterials] = material;

            delete[] scene->mMaterials;
            scene->mMaterials = newMaterials;
            scene->mNumMaterials++;

            // Copy texture to output folder
            try {
                std::filesystem::path destPath = std::filesystem::path(outputDir) / textureFilename;
                if (!std::filesystem::exists(destPath)) {
                    std::filesystem::copy(texturePath, destPath, std::filesystem::copy_options::overwrite_existing);
                    std::cout << "Copied texture to: " << destPath << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to copy texture: " << e.what() << std::endl;
            }
        }

        mesh->mMaterialIndex = materialIndex;
    }

    return mesh;
}

aiNode* CreateNodeWithMesh(aiScene* scene, const std::string& nodeName, aiMesh* mesh) {
    aiNode* node = new aiNode();
    node->mName = aiString(nodeName);
    node->mNumMeshes = 1;
    node->mMeshes = new unsigned int[1];
    node->mMeshes[0] = scene->mNumMeshes;

    // Add mesh to scene
    aiMesh** newMeshes = new aiMesh * [scene->mNumMeshes + 1];
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        newMeshes[i] = scene->mMeshes[i];
    }
    newMeshes[scene->mNumMeshes] = mesh;

    delete[] scene->mMeshes;
    scene->mMeshes = newMeshes;
    scene->mNumMeshes++;

    return node;
}

void AddChildNode(aiNode* parent, aiNode* child) {
    aiNode** newChildren = new aiNode * [parent->mNumChildren + 1];
    for (unsigned int i = 0; i < parent->mNumChildren; ++i) {
        newChildren[i] = parent->mChildren[i];
    }
    newChildren[parent->mNumChildren] = child;
    child->mParent = parent;

    delete[] parent->mChildren;
    parent->mChildren = newChildren;
    parent->mNumChildren++;
}

aiScene* CreateSceneFromTerrain(const TerrainMesh& terrain, std::string& terrainTexturePath, const std::string& outputDir) {
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mRootNode->mName = aiString("TerrainScene");
    scene->mMeshes = nullptr;
    scene->mNumMeshes = 0;
    scene->mMaterials = nullptr;
    scene->mNumMaterials = 0;

    terrainTexturePath = "terrain_color_exported.png";
    ExportTerrainTextureFromGPU(terrain, terrainTexturePath);

    MeshData* terrainData = ConvertMeshToMeshData(terrain, "Terrain");
    aiMesh* terrainMesh = ConvertMeshDataToAssimp(terrainData, scene, terrainTexturePath, outputDir);
    aiNode* terrainNode = CreateNodeWithMesh(scene, "Terrain", terrainMesh);
    AddChildNode(scene->mRootNode, terrainNode);
    delete terrainData;

    const auto& instanceMeshes = terrain.getInstanceMeshes();
    const auto& modelInstances = terrain.getModelInstances();
    int instanceIndex = 0;

    for (const auto& pair : modelInstances) {
        const std::string& modelPath = pair.first;
        const auto& transforms = pair.second;
        auto modelIt = instanceMeshes.find(modelPath);
        if (modelIt == instanceMeshes.end()) continue;

        const auto& model = modelIt->second;
        const auto& meshes = model->getMeshes();

        for (const auto& transform : transforms) {
            aiNode* groupNode = new aiNode();
            groupNode->mName = aiString("DecoGroup_" + std::to_string(instanceIndex));

            for (size_t i = 0; i < meshes.size(); ++i) {
                Mesh* mesh = meshes[i];
                MeshData* decoData = ConvertMeshToMeshDataWithTransform(*mesh, "Deco_" + std::to_string(instanceIndex) + "_Mesh_" + std::to_string(i), transform);
                std::string texture = mesh->getTexturePath();
                aiMesh* decoMesh = ConvertMeshDataToAssimp(decoData, scene, texture, outputDir);
                aiNode* decoNode = CreateNodeWithMesh(scene, decoData->name, decoMesh);
                AddChildNode(groupNode, decoNode);
                delete decoData;
            }

            AddChildNode(terrainNode, groupNode);
            instanceIndex++;
        }
    }

    return scene;
}

void ExportMeshAsOBJ(const TerrainMesh& terrain, const std::string& filename) {
    std::filesystem::path outPath(filename);
    std::string outputDir = outPath.parent_path().string();
    std::string terrainTexturePath;

    aiScene* scene = CreateSceneFromTerrain(terrain, terrainTexturePath, outputDir);
    if (!scene) {
        std::cerr << "Failed to create scene for OBJ export" << std::endl;
        return;
    }

    Assimp::Exporter exporter;
    aiReturn result = exporter.Export(scene, "obj", filename, 0);

    if (result == AI_SUCCESS) {
        std::cout << "Exported mesh to OBJ: " << filename << std::endl;
    }
    else {
        std::cerr << "OBJ export failed: " << exporter.GetErrorString() << std::endl;
    }

    delete scene;
}
