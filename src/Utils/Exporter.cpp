#include "Exporter.h"
#include <fbxsdk.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <../src/Core/stb_image_write.h>

// Extracts the resultTextureID from the TerrainMesh and saves it as a PNG
bool ExportTerrainTextureFromGPU(const TerrainMesh& terrain, const std::string& outputPath)
{
    if (terrain.getTextureID() == 0) {
        std::cerr << "ERROR: Terrain has no baked texture to export!" << std::endl;
        return false;
    }

    const int texSize = 1024; // Or query actual size if dynamic

    std::vector<unsigned char> pixels(texSize * texSize * 3); // RGB only

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

    for (const auto& v : mesh.vertices) {
        vertexData.push_back(v.Position.x);
        vertexData.push_back(v.Position.y);
        vertexData.push_back(v.Position.z);
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

FbxNode* ConvertMeshDataToFbx(FbxScene* scene, MeshData* data, const std::string& texturePath = "") {
    const int stride = 5;
    const int numVertices = static_cast<int>(data->vertices.size() / stride);

    FbxMesh* fbxMesh = FbxMesh::Create(scene, data->name.c_str());
    fbxMesh->InitControlPoints(numVertices);
    FbxVector4* controlPoints = fbxMesh->GetControlPoints();

    for (int i = 0; i < numVertices; ++i) {
        float x = data->vertices[i * stride + 0] * 100.0f;
        float y = data->vertices[i * stride + 1] * 100.0f;
        float z = data->vertices[i * stride + 2] * 100.0f;
        controlPoints[i] = FbxVector4(x, y, z);
    }

    const size_t triangleCount = data->indices.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i) {
        fbxMesh->BeginPolygon();
        fbxMesh->AddPolygon(data->indices[i * 3 + 0]);
        fbxMesh->AddPolygon(data->indices[i * 3 + 1]);
        fbxMesh->AddPolygon(data->indices[i * 3 + 2]);
        fbxMesh->EndPolygon();
    }

    FbxLayer* layer = fbxMesh->GetLayer(0);
    if (!layer) {
        fbxMesh->CreateLayer();
        layer = fbxMesh->GetLayer(0);
    }

    // UVs
    FbxGeometryElementUV* uvElement = fbxMesh->CreateElementUV("UVSet");
    uvElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
    uvElement->SetReferenceMode(FbxGeometryElement::eDirect);
    for (int i = 0; i < numVertices; ++i) {
        float u = data->vertices[i * stride + 3];
        float v = 1.0f - data->vertices[i * stride + 4];
        uvElement->GetDirectArray().Add(FbxVector2(u, v));
    }
    layer->SetUVs(uvElement, FbxLayerElement::eTextureDiffuse);

    // Normals (dummy)
    FbxGeometryElementNormal* normalElement = fbxMesh->CreateElementNormal();
    normalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
    normalElement->SetReferenceMode(FbxGeometryElement::eDirect);
    for (int i = 0; i < numVertices; ++i) {
        normalElement->GetDirectArray().Add(FbxVector4(0, 1, 0));
    }
    layer->SetNormals(normalElement);

    FbxNode* meshNode = FbxNode::Create(scene, data->name.c_str());
    meshNode->SetNodeAttribute(fbxMesh);

    // Material
    FbxSurfacePhong* material = FbxSurfacePhong::Create(scene, (data->name + "_Material").c_str());
    material->ShadingModel.Set("Phong");
    material->Diffuse.Set(FbxDouble3(1.0, 1.0, 1.0));
    material->DiffuseFactor.Set(1.0);

    if (!texturePath.empty()) {
        FbxFileTexture* texture = FbxFileTexture::Create(scene, (data->name + "_DiffuseTexture").c_str());
        texture->SetFileName(texturePath.c_str());
        texture->SetTextureUse(FbxTexture::eStandard);
        texture->SetMappingType(FbxTexture::eUV);
        texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
        texture->SetWrapMode(FbxTexture::eRepeat, FbxTexture::eRepeat);

        material->Diffuse.ConnectSrcObject(texture);
    }

    meshNode->AddMaterial(material);

    FbxLayerElementMaterial* matElement = FbxLayerElementMaterial::Create(fbxMesh, "MaterialElement");
    matElement->SetMappingMode(FbxLayerElement::eAllSame);
    matElement->SetReferenceMode(FbxLayerElement::eIndexToDirect);
    matElement->GetIndexArray().Add(0);
    layer->SetMaterials(matElement);

    return meshNode;
}

void ExportMeshAsFBX(const TerrainMesh& terrain, const std::string& filename) {
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    ios->SetBoolProp(EXP_FBX_MATERIAL, true);
    ios->SetBoolProp(EXP_FBX_TEXTURE, true);
    ios->SetBoolProp(EXP_FBX_EMBEDDED, false);
    ios->SetBoolProp(EXP_FBX_SHAPE, true);
    ios->SetBoolProp(EXP_FBX_ANIMATION, false);

    FbxScene* scene = FbxScene::Create(manager, "TerrainScene");
    scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
    scene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::OpenGL);

    FbxNode* root = scene->GetRootNode();

    // Bake terrain vertex colors to a texture
    std::string terrainTex = "terrain_color_exported.png";
    if (!ExportTerrainTextureFromGPU(terrain, terrainTex)) {
        std::cerr << "Failed to export terrain texture!" << std::endl;
    }

    MeshData* terrainData = ConvertMeshToMeshData(terrain, "Terrain");
    FbxNode* terrainNode = ConvertMeshDataToFbx(scene, terrainData, terrainTex);
    root->AddChild(terrainNode);

    // Decorations
    const auto& instanceMeshes = terrain.getInstanceMeshes();
    const auto& modelInstances = terrain.getModelInstances();
    int instanceIndex = 0;

    for (const auto& pair : modelInstances) {
        const std::string& modelPath = pair.first;
        const std::vector<glm::mat4>& transforms = pair.second;

        auto modelIt = instanceMeshes.find(modelPath);
        if (modelIt == instanceMeshes.end()) continue;

        const auto& model = modelIt->second;
        const auto& meshes = model->getMeshes(); // likely vector<Mesh*>

        for (const auto& transform : transforms) {
            FbxNode* groupNode = FbxNode::Create(scene, ("DecoGroup_" + std::to_string(instanceIndex)).c_str());

            for (size_t i = 0; i < meshes.size(); ++i) {
                Mesh* mesh = meshes[i]; // pointer

                MeshData* decoData = ConvertMeshToMeshDataWithTransform(*mesh, "Deco_" + std::to_string(instanceIndex) + "_Mesh_" + std::to_string(i), transform);
                std::string texture = mesh->getTexturePath(); // if mesh is Mesh*
                FbxNode* decoNode = ConvertMeshDataToFbx(scene, decoData, texture);
                groupNode->AddChild(decoNode);
                delete decoData;
            }

            terrainNode->AddChild(groupNode);
            instanceIndex++;
        }
    }


    FbxExporter* exporter = FbxExporter::Create(manager, "");
    if (exporter->Initialize(filename.c_str(), -1, manager->GetIOSettings())) {
        exporter->Export(scene);
        std::cout << "Exported FBX to: " << filename << std::endl;
    }
    else {
        std::cerr << "Export failed: " << exporter->GetStatus().GetErrorString() << std::endl;
    }

    exporter->Destroy();
    scene->Destroy();
    ios->Destroy();
    manager->Destroy();
}