#include "Application.h"

Application* Application::_instance = nullptr;

Application::Application() {
    m_window = new AppWindow();

    init();
}

Application::~Application() {
    shutdown();

    delete m_window;
    delete terrainController;
}

void Application::init() {
    if (!m_window->init()) {
        std::cerr << "Failed to initialize window" << std::endl;
        m_isRunning = false;
        return;
    }

    LoadDefaultShaders();

    sceneView.init(m_window);
    sceneView.setCamera(&camera);
    sceneView.setRenderer(&renderer);
    uiCtx.init(m_window);

    renderer.SetCamera(&camera);

    terrainController = new TerrainController(&renderer);
}

void Application::shutdown() {
    uiCtx.shutdown();
    m_window->shutdown();
}

void Application::LoadDefaultShaders() {
    ShaderManager* shaderManager = ShaderManager::GetInstance();
    shaderManager->loadShader("solid", "Shaders/VertexShader.vs", "Shaders/solid.fs");
    shaderManager->loadShader("unshaded", "Shaders/VertexShader.vs", "Shaders/FragmentShader.fs");
    shaderManager->loadShader("terrain", "Shaders/Terrain.vert", "Shaders/Terrain.frag");
    shaderManager->loadShader("terrainTexture", "Shaders/TerrainTexture.vert", "Shaders/TerrainTexture.frag");
}

void Application::Run() {
    while (m_isRunning && !m_window->shouldClose()) {
        Utils::Time::Update();
        m_window->newFrame();

        renderer.ClearQueue();

        uiCtx.preRender();

        uiCtx.render();	                    // Renders the Main Docking Window
        terrainController->DisplayUI();	    // Renders the TerrainUI Windows
        terrainController->Update();        // pushes the terrain data to Renderer

        m_window->clear();                  // Clears the Window
        sceneView.render();		            // Renders the Scene Window

        uiCtx.postRender();

        m_window->onUpdate();
    }
}
