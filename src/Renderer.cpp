#include "Renderer.h"

Renderer::Renderer() : currentShader(nullptr), currentCamera(nullptr) {}

Renderer::~Renderer() {
    // Cleanup if necessary
}

void Renderer::ClearQueue() {
    renderQueue.clear();
}

void Renderer::Render() {
    if (!currentShader || !currentCamera) return;

    for (IDrawable* mesh : renderQueue) {
        if (mesh) {
            glm::mat4 model = glm::mat4(1.0f);
            glm::mat4 view = currentCamera->GetViewMatrix();
            glm::mat4 projection = glm::perspective(glm::radians(currentCamera->zoom), (float)screenSize.x / (float)screenSize.y, 0.1f, 1000.0f);

            currentShader->use();
            currentShader->setMat4("model", model);
            currentShader->setMat4("view", view);
            currentShader->setMat4("projection", projection);

            mesh->Draw(*currentShader);
        }
    }

    renderQueue.clear(); // Clear the render queue after rendering
}

void Renderer::AddToRenderQueue(IDrawable* mesh) {
    if (mesh) {
        renderQueue.push_back(mesh);
    }
}

void Renderer::SetShader(Shader* shader) {
    currentShader = shader;
}

void Renderer::SetCamera(Camera* camera) {
    currentCamera = camera;
}