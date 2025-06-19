#pragma once

#include <vector>

#include "../Core/Texture.h"
#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace std;

class Model : public IDrawable {
public:
	Model(const char* path, bool gamma = false) : gammaCorrection(gamma)
	{
		loadModel(path);
	}
	~Model() {
		for (auto mesh : meshes)
			delete mesh;
	}
	void Draw(Shader& shader) override;
	void Draw(const glm::mat4& view, const glm::mat4& projection) override;
	std::vector<Mesh*> getMeshes() {
		return meshes;
	}	
	
	void DrawInstanced(const glm::mat4& view, const glm::mat4& projection, const std::vector<glm::mat4>& instances);

	void SetShader(std::shared_ptr<Shader> shader) override;

private:
	vector<Mesh*>	meshes;
	vector<std::shared_ptr<Texture>> textures_loaded;
	string directory;
	bool gammaCorrection;
	void loadModel(string path);

	void processNode(aiNode* node, const aiScene* scene);
	Mesh* processMesh(aiMesh* mesh, const aiScene* scene);
	vector<std::shared_ptr<Texture>> loadMaterialTextures(aiMaterial* mat, aiTextureType type, TexType typeName);

};
