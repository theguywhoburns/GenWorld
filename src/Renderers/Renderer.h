#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "../Drawables/IDrawable.h"
#include "../Core/Shader.h"
#include "../Core/Camera.h"
#include "../Core/Framebuffer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Renderer {
public:
    ~Renderer();

    void Render();
    void ClearQueue();
    void AddToRenderQueue(IDrawable* mesh);

    void SetShader(Shader* shader);
    void SetCamera(Camera* camera);

    void SetScreenSize(glm::vec2 size);
    void SetScreenSize(float width, float height);
    glm::vec2 GetScreenSize();

private:
    void renderScene();

    glm::vec2 screenSize;
    std::vector<IDrawable*> renderQueue;
    Shader* currentShader;
    Camera* currentCamera;
    FrameBuffer framebuffer;

};

#endif