#pragma once

#include "IRenderContext.h"
#include "../Core/Framebuffer.h"
#include "../Core/Camera.h"
#include "Renderer.h"

class SceneContext : public IRenderContext {
public:
    ~SceneContext() override = default;

    bool init(GLFWwindow* window) override;
    void shutdown() override;
    void preRender() override;
    void postRender() override;
    void render() override;

    void setCamera(Camera* camera) { this->camera = camera; }
    void setRenderer(Renderer* renderer) { this->renderer = renderer; }
    GLFWwindow* getWindow() const { return window; }

private:
    Renderer* renderer;
    FrameBuffer framebuffer;
    Camera* camera;

    unsigned int SCR_WIDTH = 1024;
    unsigned int SCR_HEIGHT = 768;

};