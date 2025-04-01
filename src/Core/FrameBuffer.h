#pragma once

// GL includes
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <iostream>

class FrameBuffer {
private:
    unsigned int m_FramebufferID = 0;
    unsigned int m_ColorTextureID = 0;
    unsigned int m_DepthStencilID = 0;
    int m_Width = 0;
    int m_Height = 0;

public:
    ~FrameBuffer() {
        Destroy();
    }

    void Resize(int width, int height);

    void bind();

    void unbind();

    void Destroy();

    unsigned int GetColorTextureID() const { return m_ColorTextureID; }
    unsigned int GetFramebufferID() const { return m_FramebufferID; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
};