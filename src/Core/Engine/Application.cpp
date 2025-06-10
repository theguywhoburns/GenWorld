#include "Application.h"
#include "../../Controllers/BlockController.h"
#include "../../Controllers/TerrainController.h"
#include "../../Generators/BlockGenerator.h"
#include "../../Generators/TerrainGenerator.h"
#include <iostream>

Application* Application::_instance = nullptr;

Application::Application() {
    m_window = new Window();

    init();
}

Application::~Application() {
    shutdown();

    delete m_window;
    delete generatorController;
}

void Application::init() {
    std::cout << "Starting window initialization..." << std::endl;
    if (!m_window->init()) {
        std::cerr << "Failed to initialize window" << std::endl;
        m_isRunning = false;
        return;
    }
    std::cout << "Window initialized successfully" << std::endl;

    std::cout << "Loading shaders..." << std::endl;
    LoadDefaultShaders();
    std::cout << "Shaders loaded" << std::endl;

    std::cout << "Initializing scene view..." << std::endl;
    sceneView.init(m_window);
    sceneView.setCamera(&camera);
    sceneView.setRenderer(&renderer);
    std::cout << "Scene view initialized" << std::endl;

    std::cout << "Initializing UI context..." << std::endl;
    uiCtx.init(m_window);
    std::cout << "UI context initialized" << std::endl;

    renderer.SetCamera(&camera);

    std::cout << "Creating BlockController..." << std::endl;
    generatorController = new BlockController(&renderer);
    std::cout << "BlockController created successfully" << std::endl;
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
        generatorController->DisplayUI();	    // Renders the TerrainUI Windows
        generatorController->Update();        // pushes the terrain data to Renderer
        sceneView.render();		            // Renders the Scene Window

        uiCtx.postRender();

        m_window->onUpdate();
    }
}
