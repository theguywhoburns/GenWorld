#include "Exporter.h"
#include <fbxsdk.h>
#include <iostream>

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
    MeshData* data = new MeshData(name, vertexData, indexData);
    return data;
}

void ExportMeshAsFBX(const Mesh& mesh, const std::string& filename) {
    MeshData* meshData = ConvertMeshToMeshData(mesh, "TerrainMesh");
    ExportMeshDataAsFBX(meshData, filename);
    delete meshData;
}


void ExportMeshDataAsFBX(MeshData* data, const std::string& filename) {
    if (!data || data->vertices.empty() || data->indices.empty()) {
        std::cerr << "MeshData is empty or invalid." << std::endl;
        return;
    }

    // Constants
    const int stride = 5; // x, y, z, u, v
    const int numVertices = static_cast<int>(data->vertices.size() / stride);

    // Create FBX manager and scene
    FbxManager* manager = FbxManager::Create();
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    FbxScene* scene = FbxScene::Create(manager, "ExportedScene");
    scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
    scene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::OpenGL);

    // Create mesh
    FbxMesh* fbxMesh = FbxMesh::Create(scene, data->name.c_str());
    fbxMesh->InitControlPoints(numVertices);
    FbxVector4* controlPoints = fbxMesh->GetControlPoints();

    // Assign positions
    for (int i = 0; i < numVertices; ++i) {
        float x = data->vertices[i * stride + 0] * 100.0f; // Convert to cm
        float y = data->vertices[i * stride + 1] * 100.0f;
        float z = data->vertices[i * stride + 2] * 100.0f;
        controlPoints[i] = FbxVector4(x, y, z);
    }

    // Prepare UV layer
    FbxLayer* layer = fbxMesh->GetLayer(0);
    if (!layer) {
        fbxMesh->CreateLayer();
        layer = fbxMesh->GetLayer(0);
    }

    FbxLayerElementUV* uvLayer = FbxLayerElementUV::Create(fbxMesh, "UVSet");
    uvLayer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
    uvLayer->SetReferenceMode(FbxLayerElement::eDirect);

    // Create triangles
    const size_t triangleCount = data->indices.size() / 3;
    for (size_t i = 0; i < triangleCount; ++i) {
        int idx0 = data->indices[i * 3 + 0];
        int idx1 = data->indices[i * 3 + 1];
        int idx2 = data->indices[i * 3 + 2];

        fbxMesh->BeginPolygon();
        fbxMesh->AddPolygon(idx0);
        fbxMesh->AddPolygon(idx1);
        fbxMesh->AddPolygon(idx2);
        fbxMesh->EndPolygon();

        // Add UVs per index (eByPolygonVertex)
        for (int j = 0; j < 3; ++j) {
            int idx = data->indices[i * 3 + j];
            float u = data->vertices[idx * stride + 3];
            float v = data->vertices[idx * stride + 4];
            uvLayer->GetDirectArray().Add(FbxVector2(u, v));
        }
    }

    layer->SetUVs(uvLayer, FbxLayerElement::eTextureDiffuse);

    // Create node and attach mesh
    FbxNode* meshNode = FbxNode::Create(scene, "MeshNode");
    meshNode->SetNodeAttribute(fbxMesh);
    scene->GetRootNode()->AddChild(meshNode);

    // Export
    FbxExporter* exporter = FbxExporter::Create(manager, "");
    if (exporter->Initialize(filename.c_str(), -1, manager->GetIOSettings())) {
        exporter->Export(scene);
        std::cout << "Exported FBX to: " << filename << std::endl;
    }
    else {
        std::cerr << "FBX export failed: " << exporter->GetStatus().GetErrorString() << std::endl;
    }

    // Cleanup
    exporter->Destroy();
    scene->Destroy();
    ios->Destroy();
    manager->Destroy();
}