#include "Application.h"
#include "../../Controllers/BlockController.h"
#include "../../Controllers/TerrainController.h"
#include "../../Generators/BlockGenerator.h"
#include "../../Generators/TerrainGenerator.h"
#include <iostream>

Application* Application::_instance = nullptr;

void Application::Generate() {
    if (generatorController != nullptr) {
        generatorController->Generate();
    }
}

void Application::RandomizeSeed() {
    if (generatorController != nullptr) {
        generatorController->RandomizeSeed();
    }
}

void Application::Export(const std::string& format) {
    if (generatorController == nullptr) {
        std::cerr << "Generator controller is not initialized." << std::endl;
        return;
    }

    Mesh* mesh = generatorController->getGenerator().GetMesh();
    Utils::MeshExporter::ExportMesh(mesh, format);
}

Application::Application() {
    m_window = new AppWindow();

    init();
}

Application::~Application() {
    shutdown();

    delete m_window;
    delete terrainController;
    delete blockController;
    generatorController = nullptr;
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

    uiCtx.setCamera(&camera);
    renderer.SetCamera(&camera);

    blockController = new BlockController(&renderer);
    terrainController = new TerrainController(&renderer);

    generatorController = terrainController;
}

void Application::CheckGenerationMode() {
    int mode = uiCtx.getMode();

    // Always keep generatorController in sync with mode
    if (mode == 0) {
        generatorController = blockController;
    } else {
        generatorController = terrainController;
    }
}

void Application::shutdown() {
    uiCtx.shutdown();
    m_window->shutdown();
}

void Application::LoadDefaultShaders() {
    ShaderManager* shaderManager = ShaderManager::GetInstance();
    shaderManager->loadShader("solid", "Shaders/VertexShader.vs", "Shaders/solid.fs");
    shaderManager->loadShader("rendered", "Shaders/VertexShader.vs", "Shaders/FragmentShader.fs");
    shaderManager->loadShader("terrain", "Shaders/Terrain.vert", "Shaders/Terrain.frag");
    shaderManager->loadShader("terrainTexture", "Shaders/TerrainTexture.vert", "Shaders/TerrainTexture.frag");
    shaderManager->loadShader("wireframe", "Shaders/Wireframe.vs", "Shaders/Wireframe.fs", "Shaders/Wireframe.gs");
}

void Application::Run() {
    while (m_isRunning && !m_window->shouldClose()) {
        Utils::Time::Update();
        CheckGenerationMode();

        m_window->newFrame();

        renderer.ClearQueue();

        uiCtx.preRender();

        
        uiCtx.render();	                    // Renders the Main Docking Window
        generatorController->DisplayUI();	    // Renders the TerrainUI Windows
        generatorController->Update();        // pushes the terrain data to Renderer

        m_window->clear();                  // Clears the Window
        sceneView.render();		            // Renders the Scene Window

        uiCtx.postRender();

        m_window->onUpdate();
    }
}