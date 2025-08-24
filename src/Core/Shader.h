#ifndef SHADER_H
#define SHADER_H

#include <fstream>
#include <glad/gl.h> // include glad to get all the required OpenGL headers
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

class Shader {
public:
  // the program ID
  unsigned int ID;

  // constructor reads and builds the shader
  Shader(const char *vertexPath, const char *fragmentPath);
  Shader(const char *vertexPath, const char *fragmentPath,
         const char *geometryPath);
  ~Shader();
  // use/activate the shader
  void use();
  // utility uniform functions
  void setBool(const std::string &name, bool value) const;
  void setInt(const std::string &name, int value) const;
  void setFloat(const std::string &name, float value) const;

  void setMat3(const std::string &name, const glm::mat3 &mat) const;
  void setMat4(const std::string &name, const glm::mat4 &mat) const;

  void setVec2(const std::string &name, const float x, const float y) const;
  void setVec3(const std::string &name, const float x, const float y,
               const float z) const;
  void setVec4(const std::string &name, const float x, const float y,
               const float z, const float w) const;

  void setVec2(const std::string &name, const glm::vec2 &value) const;
  void setVec3(const std::string &name, const glm::vec3 &value) const;
  void setVec4(const std::string &name, const glm::vec4 &value) const;

private:
  void checkCompileErrors(unsigned int shader, std::string type);
};

#endif