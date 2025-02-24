#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include <Drawables/IDrawable.h>
#include <Shader.h>
#include <Camera.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();

    void ClearQueue();
    void Render();
    void AddToRenderQueue(IDrawable* mesh);
    void SetShader(Shader* shader);
    void SetCamera(Camera* camera);
    void SetScreenSize(glm::vec2 size);
    glm::vec2 GetScreenSize();

private:
    glm::vec2 screenSize;
    std::vector<IDrawable*> renderQueue;
    Shader* currentShader;
    Camera* currentCamera;
};

#endif