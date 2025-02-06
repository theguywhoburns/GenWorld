#pragma once

#include <vector>

#include <Texture.h>
#include <Shader.h>
#include <Mesh.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <map>

using namespace std;

class Model {
public:
	Model(const char* path, bool gamma = false) : gammaCorrection(gamma)
	{
		loadModel(path);
	}
	void Draw(Shader& shader);

private:
	vector<Mesh>	meshes;
	vector<Texture> textures_loaded;
	string directory;
	bool gammaCorrection;

	void loadModel(string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, TexType typeName);

};
