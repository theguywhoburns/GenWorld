#include <GenWorld/Core/Texture.h>
#include <GenWorld/Utils/Utils.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <GenWorld/Core/stb_image.h>
#include <GenWorld/Utils/OpenGlInc.h>
Texture::Texture(std::string path, TexType type) {
  this->type = type;
  this->path = Utils::NormalizePath(path);

  glGenTextures(1, &ID);
  bind();

  // tell stb_image.h to flip loaded texture's on the y-axis.
  // stbi_set_flip_vertically_on_load(true);

  unsigned char *data =
      stbi_load(this->path.c_str(), &width, &height, &nrChannels, 0);
  if (data) {
    GLenum format = GL_RGB;
    if (nrChannels == 1)
      format = GL_RED;
    else if (nrChannels == 3)
      format = GL_RGB;
    else if (nrChannels == 4)
      format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture " << path << std::endl;
  }
  stbi_image_free(data);

  unbind();
}

Texture::~Texture() { glDeleteTextures(1, &ID); }

void Texture::activate(GLenum textureUnit) { glActiveTexture(textureUnit); }

void Texture::bind() { glBindTexture(GL_TEXTURE_2D, ID); }

void Texture::unbind() { glBindTexture(GL_TEXTURE_2D, 0); }
