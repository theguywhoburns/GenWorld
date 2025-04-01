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
    const char* title = "GenWorld";

    unsigned int SCR_WIDTH = 1024;
    unsigned int SCR_HEIGHT = 768;

    struct MousePosition {
        double x, y;
    };

    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    MousePosition cursorPos = { 0.0, 0.0 };

    bool camMode = false;
    bool isSceneWindowHovered = false;
    float lastX = 0, lastY = 0;

    void calculateDeltaTime();
    void processInput();
    double calculateFPS();
    void calculateMousePos();
    void updateTitle(double fps);
    void mouseClick();
    void mouse_pos_calc();

};
