#include "Renderer.h"

Renderer::Renderer() {}

Renderer::~Renderer() {
    // Cleanup if necessary
}

void Renderer::ClearQueue() {
    renderQueue.clear();
}

void Renderer::Render() {
    if (currentShader == nullptr || currentCamera == nullptr) return;

    glm::mat4 view = currentCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(currentCamera->zoom), (float)screenSize.x / (float)screenSize.y, 0.1f, 1000.0f);

    for (IDrawable* mesh : renderQueue) {
        if (mesh != nullptr) {
            glm::mat4 model = glm::mat4(1.0f);

            currentShader->use();
            currentShader->setMat4("model", model);
            currentShader->setMat4("view", view);
            currentShader->setMat4("projection", projection);

            mesh->Draw(*currentShader);
        }
    }
}

void Renderer::AddToRenderQueue(IDrawable* mesh) {
    if (mesh != nullptr) {
        renderQueue.push_back(mesh);
    }
}

void Renderer::SetShader(Shader* shader) {
    currentShader = shader;
}

void Renderer::SetCamera(Camera* camera) {
    currentCamera = camera;
}

void Renderer::SetScreenSize(glm::vec2 size) {
    screenSize = size;
}

void Renderer::SetScreenSize(float width, float height) {
    screenSize = glm::vec2(width, height);
}

glm::vec2 Renderer::GetScreenSize() {
    return screenSize;
}