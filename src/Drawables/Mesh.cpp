#include "Mesh.h"

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<std::shared_ptr<Texture>> textures) {
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	setupMesh();
}

Mesh::~Mesh() {
	if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
	if (indexBuffer) glDeleteBuffers(1, &indexBuffer);
	if (arrayObj) glDeleteVertexArrays(1, &arrayObj);
	if (instancingInitialized) glDeleteBuffers(1, &instanceVBO);

	vertexBuffer = 0;
	indexBuffer = 0;
	arrayObj = 0;
}

void Mesh::Draw(Shader& shader) {
	bindTextures(shader);
	glBindVertexArray(arrayObj);
	glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	unbindTextures();
}

void Mesh::Draw(const glm::mat4& view, const glm::mat4& projection) {
	if (m_shader != nullptr) {
		m_shader->use();

		glm::mat4 model = transform.getModelMatrix();
		m_shader->setMat4("uModel", model);
		m_shader->setMat4("uView", view);
		m_shader->setMat4("uProjection", projection);

		// viewport shading uniforms

		Draw(*m_shader);
	}
}

void Mesh::DrawInstanced(unsigned int instanceCount, const glm::mat4& view, const glm::mat4& projection) {
	m_shader->use();

	m_shader->setMat4("uModel", glm::mat4(1.0f));
	m_shader->setMat4("uView", view);
	m_shader->setMat4("uProjection", projection);

	bindTextures(*m_shader);

	glBindVertexArray(arrayObj);
	glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instanceCount);
	glBindVertexArray(0);

	unbindTextures();
}

void Mesh::setupMesh() {
	glGenVertexArrays(1, &arrayObj);
	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &indexBuffer);
	glGenBuffers(1, &instanceVBO);

	glBindVertexArray(arrayObj);
	// load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	// A great thing about structs is that their memory layout is sequential for all its items.
	// The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
	// again translates to 3/2 floats which translates to a byte array.
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// set the vertex attribute pointers
	// vertex Positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	// vertex colors
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
	// vertex texture coords
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// vertex tangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	// vertex bitangent
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

	// ids
	glEnableVertexAttribArray(6);
	glVertexAttribIPointer(6, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
	// weights
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

	// Setup identity matrix for instance attributes (locations 8-11)
	float identityMatrix[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,  // instanceModel0
		0.0f, 1.0f, 0.0f, 0.0f,  // instanceModel1  
		0.0f, 0.0f, 1.0f, 0.0f,  // instanceModel2
		0.0f, 0.0f, 0.0f, 1.0f   // instanceModel3
	};

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(identityMatrix), identityMatrix, GL_STATIC_DRAW);

	// Instance matrix attributes (locations 8-11)
	// default to identity matrix in case of regular rendering
	std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; ++i) {
		glEnableVertexAttribArray(8 + i);
		glVertexAttribPointer(8 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
		glVertexAttribDivisor(8 + i, 1);
	}

	glBindVertexArray(0);
}

void Mesh::bindTextures(Shader& shader) {
	unsigned int diffN = 1;
	unsigned int specN = 1;
	unsigned int normN = 1;
	unsigned int emissionN = 1;
	unsigned int heightN = 1;

	for (unsigned int i = 0; i < textures.size(); i++) {
		Texture::activate(GL_TEXTURE0 + i);

		string number = "0";
		string type = "diffuse";
		TexType name = textures[i]->type;
		if (name == TexType::diffuse) {
			type = "diffuse";
			number = std::to_string(diffN++);
		}
		else if (name == TexType::specular) {
			type = "specular";
			number = std::to_string(specN++);
		}
		else if (name == TexType::normal) {
			type = "normal";
			number = std::to_string(normN++);
		}
		else if (name == TexType::emission) {
			type = "emission";
			number = std::to_string(emissionN++);
		}
		else if (name == TexType::height) {
			type = "height";
			number = std::to_string(heightN++);
		}

		std::string res = type + number;
		m_shader->setInt(res.c_str(), i);

		textures[i]->bind();
	}
}

void Mesh::unbindTextures() {
	Texture::activate(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Mesh::InitializeInstanceBuffer() {
	if (instancingInitialized) return;

	glDeleteBuffers(1, &instanceVBO);	// clean up any existing instance VBO

	glGenBuffers(1, &instanceVBO);
	glBindVertexArray(arrayObj);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	// Set attribute layout for mat4 (locations 8 to 11)
	std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; ++i) {
		glEnableVertexAttribArray(8 + i);
		glVertexAttribPointer(8 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));
		glVertexAttribDivisor(8 + i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	instancingInitialized = true;
}

void Mesh::UpdateInstanceData(const std::vector<glm::mat4>& instanceMatrices) {
	if (!instancingInitialized) InitializeInstanceBuffer();

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}