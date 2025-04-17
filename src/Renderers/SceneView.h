#pragma once

#include "../Core/Framebuffer.h"
#include "../Core/Camera.h"
#include "../Utils/Time.h"
#include "Renderer.h"
#include "../Core/Application.h"

class SceneView {
public:
    SceneView() {
        camera = Application::getInstance()->getCamera();
        window = Application::getInstance()->getWindow();
    }

    void preRender();
    void postRender();
    void render();

    void setRenderer(Renderer* renderer) { this->renderer = renderer; }

private:
    Renderer* renderer;
    FrameBuffer framebuffer;
    Camera* camera;

    Window* window;

    bool camMode = false;
    bool isSceneWindowHovered = false;
    float lastX = 0, lastY = 0;

    void processInput();
    void calculateMousePos();
    void mouseClick();
    void mouse_pos_calc();

};
