#include "FrameBuffer.h"

void FrameBuffer::Resize(int width, int height) {
  // Prevent invalid dimensions
  width = std::max(1, width);
  height = std::max(1, height);

  // Skip if already the right size
  if (m_Width == width && m_Height == height && m_ColorTextureID != 0)
    return;

  // Store new dimensions
  m_Width = width;
  m_Height = height;

  // Clean up existing framebuffer resources
  if (m_ColorTextureID != 0) {
    glDeleteTextures(1, &m_ColorTextureID);
    m_ColorTextureID = 0;
  }

  if (m_DepthStencilID != 0) {
    glDeleteRenderbuffers(1, &m_DepthStencilID);
    m_DepthStencilID = 0;
  }

  if (m_FramebufferID == 0) {
    glGenFramebuffers(1, &m_FramebufferID);
  }

  // Bind framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID);

  // Color attachment
  glGenTextures(1, &m_ColorTextureID);
  glBindTexture(GL_TEXTURE_2D, m_ColorTextureID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_ColorTextureID, 0);

  // Depth and stencil attachment
  glGenRenderbuffers(1, &m_DepthStencilID);
  glBindRenderbuffer(GL_RENDERBUFFER, m_DepthStencilID);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, m_DepthStencilID);

  // Check if framebuffer is complete
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "Framebuffer is not complete!" << std::endl;
  }

  // Unbind framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::bind() { glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferID); }

void FrameBuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void FrameBuffer::Destroy() {
  if (m_ColorTextureID != 0) {
    glDeleteTextures(1, &m_ColorTextureID);
    m_ColorTextureID = 0;
  }
  if (m_DepthStencilID != 0) {
    glDeleteRenderbuffers(1, &m_DepthStencilID);
    m_DepthStencilID = 0;
  }
  if (m_FramebufferID != 0) {
    glDeleteFramebuffers(1, &m_FramebufferID);
    m_FramebufferID = 0;
  }
  m_Width = m_Height = 0;
}
