#ifndef APPLICATION_H
#define APPLICATION_H
#define GLFW_INCLUDE_NONE
#include <glfw/glfw3.h>
#include <glad/glad.h>
#include "Camera.h"
#include "Window.h"
#include "../Renderers/SceneView.h"
#include "../Renderers/Renderer.h"
#include "../Renderers/UiContext.h"

class Application {
private:
    static Application* instance;
    SceneView* sceneCtx;
    Window* window;
    Camera* camera;
    Renderer* renderer;
    TerrainController* terrainController;
    UiContext* uiRenderer;

    Application() {}
public:
    static Application* getInstance();
    bool init();
    void run();
    void terminate();
    GLFWwindow* getNativeWindow();
    Window* getWindow() { return window; }
    Camera* getCamera();
};


#endif