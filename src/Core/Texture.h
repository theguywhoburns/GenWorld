#ifndef TEXTURE_H
#define TEXTURE_H

#include "../Utils/FileDialogs.h"
#include <glad/gl.h>
#include <iostream>

enum TexType { diffuse, specular, normal, height, emission };

class Texture {
public:
  unsigned int ID;
  int width, height, nrChannels;
  TexType type;
  std::string path;

  unsigned int getID() const { return ID; }

  Texture(std::string path, TexType type = TexType::diffuse);
  ~Texture();
  static void activate(GLenum textureUnit);
  void bind();
  void unbind();
};

#endif