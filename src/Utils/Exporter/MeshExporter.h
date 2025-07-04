#pragma once

#include "../MeshData.h"
#include <../../src/Core/stb_image_write.h>
#include "../../src/Drawables/Mesh.h"
#include "../../src/Drawables/TerrainMesh.h"
#include "../../src/Drawables/BlockMesh.h"
#include <assimp/scene.h>

namespace Utils {
    class MeshExporter {
    public:
        static void ExportMesh(const Mesh* mesh, std::string format);
    private:
        static void ExportMeshAsObj(const Mesh* mesh, const std::string& filename);
        // more File Extensions
    };

    class OBJTerrainExporter {
    public:
        static void ExportTerrainAsOBJ(const TerrainMesh& terrain, const std::string& filename);
    private:
        static bool ExportTerrainTextureFromGPU(const TerrainMesh& terrain, const std::string& outputPath);

        static MeshData* ConvertMeshToMeshData(const Mesh& mesh, const std::string& name);
        static MeshData* ConvertMeshToMeshDataWithTransform(const Mesh& mesh, const std::string& name, const glm::mat4& transform);
        static aiMesh* ConvertMeshDataToAssimp(MeshData* data, aiScene* scene, const std::string& texturePath, const std::string& outputDir);
        static aiNode* CreateNodeWithMesh(aiScene* scene, const std::string& nodeName, aiMesh* mesh);
        static aiScene* CreateSceneFromTerrain(const TerrainMesh& terrain, std::string& terrainTexturePath, const std::string& outputDir);
        static void AddChildNode(aiNode* parent, aiNode* child);
    };

    class OBJBlockExporter {
    };
}