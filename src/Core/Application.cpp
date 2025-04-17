
#include "Application.h"
#include "../Controllers/TerrainController.h"

Application *Application::getInstance()
{
    if (instance == nullptr) {
        instance = new Application();
    }

    return instance;
}

void Application::terminate()
{        
	uiRenderer->shutdown();

    glfwTerminate();

}

Application::~Application() {
    delete window;
    delete camera;
    delete renderer;
    delete sceneCtx;
    delete uiRenderer;
}

bool Application::init() {
    // Create a windowed mode window and its OpenGL context
    window = new Window(1024, 768, "GenWorld");
    if (!window->init()) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    // Set up camera
    camera = new Camera(glm::vec3(0.0f, 20.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
    
    renderer = new Renderer();

    Shader ourShader("Shaders/Terrain.vert", "Shaders/Terrain.frag");
	renderer->SetShader(&ourShader);

    terrainController = new TerrainController(renderer);

    sceneCtx->setRenderer(renderer);

    uiRenderer = new UiContext();

	uiRenderer->init();

    return true;
}

void Application::run() {
    while (!window->shouldClose()) {

		Utils::Time::Update();

		renderer->ClearQueue();

		uiRenderer->preRender();
		sceneCtx->preRender();

		uiRenderer->render();	// Renders the Main Docking Window

		terrainController->DisplayUI();	// Renders the TerrainUI Windows
		terrainController->Update();

		sceneCtx->render();		// Renders the Scene Window

		uiRenderer->postRender();
		sceneCtx->postRender();

        window->onUpdate();
    }
}

Camera* Application::getCamera()
{
    return camera;
}