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

    std::cout << "Creating Controllers..." << std::endl;

    blockController = new BlockController(&renderer);
    terrainController = new TerrainController(&renderer);

    generatorController = blockController;

    std::cout << "Controllers created successfully" << std::endl;
}

void Application::RenderTopBar() {
    static int mode = 0; // 0 = Block Generation, 1 = Terrain Generation

    // Main menu bar (at the very top)
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Mode")) {
            if (ImGui::MenuItem("Block Generation", nullptr, mode == 0)) {
                mode = 0;
            }
            if (ImGui::MenuItem("Terrain Generation", nullptr, mode == 1)) {
                mode = 1;
            }
            ImGui::EndMenu();
        }
        ImGui::SameLine();
        ImGui::Text("| Current: %s", mode == 0 ? "Block Generation" : "Terrain Generation");
        ImGui::EndMainMenuBar();
    }

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
        RenderTopBar();	                    // Renders the top menu bar
        generatorController->Update();        // pushes the terrain data to Renderer
        sceneView.render();		            // Renders the Scene Window

        uiCtx.postRender();

        m_window->onUpdate();
    }
}
