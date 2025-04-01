#include "Mesh.h"

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	setupMesh();
}

Mesh::~Mesh()
{
	if (vertexBuffer) glDeleteBuffers(1, &vertexBuffer);
	if (indexBuffer) glDeleteBuffers(1, &indexBuffer);
	if (arrayObj) glDeleteVertexArrays(1, &arrayObj);

	vertexBuffer = 0;
	indexBuffer = 0;
	arrayObj = 0;
}

void Mesh::Draw(Shader& shader)
{
	unsigned int diffN = 1;
	unsigned int specN = 1;
	unsigned int normN = 1;
	unsigned int emissionN = 1;
	unsigned int heightN = 1;

	for (unsigned int i = 0; i < textures.size(); i++) {
		Texture::activate(GL_TEXTURE0 + i);

		string number = "0";
		string type = "diffuse";
		TexType name = textures[i].type;
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

		std::string res = "material." + type + number;
		shader.setInt(res.c_str(), i);

		textures[i].bind();
	}

	glBindVertexArray(arrayObj);
	glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	Texture::activate(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Mesh::setupMesh()
{
	glGenVertexArrays(1, &arrayObj);
	glGenBuffers(1, &vertexBuffer);
	glGenBuffers(1, &indexBuffer);

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
	// vertex texture coords
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	// vertex tangent
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	// vertex bitangent
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

	// ids
	glEnableVertexAttribArray(5);
	glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
	// weights
	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

	glBindVertexArray(0);
}
