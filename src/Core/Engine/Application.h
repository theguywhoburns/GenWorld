#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../Renderers/UiContext.h"
#include "../../Renderers/SceneView.h"
#include "../../Renderers/Renderer.h"
#include "../../Generators/TerrainGenerator.h"
#include "../../Controllers/TerrainController.h"
#include "../../Utils/Time.h"

#include "../Shader.h"
#include "../Camera.h"
#include "Window.h"

class TerrainController;

class Application {
public:
    void Run();

    static Application* GetInstance() {
        if (_instance == nullptr) {
            _instance = new Application();
        }
        return _instance;
    }

    Window* GetWindow() const { return m_window; }

private:
    Application();
    ~Application();

    void init();
    void shutdown();

    static Application* _instance;

    bool m_isRunning = true;

    Window* m_window;
    Camera camera = Camera(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, -45.0f);
    Shader* ourShader;
    Renderer renderer;
    SceneView sceneView;
    UiContext uiCtx;
    TerrainController* terrainController;

    friend int main(void);
};
