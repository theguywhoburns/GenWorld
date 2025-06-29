#include "Renderer.h"

Renderer::~Renderer() {
    // Cleanup if necessary
}

void Renderer::ClearQueue() {
    renderQueue.clear();
}

void Renderer::Render() {
    renderScene();
}

void Renderer::AddToRenderQueue(IDrawable* mesh) {
    if (mesh != nullptr) {
        renderQueue.push_back(mesh);
    }
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

void Renderer::renderScene() {
    if (currentCamera == nullptr) return;

    glm::mat4 view = currentCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(currentCamera->zoom), (float)screenSize.x / (float)screenSize.y, 0.1f, 1000.0f);

    for (IDrawable* mesh : renderQueue) {
        if (mesh != nullptr) {
            mesh->SetShaderParameters(currentShadingParams);
            mesh->Draw(view, projection);
        }
    }
}
