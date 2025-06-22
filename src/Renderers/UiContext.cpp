#include "UiContext.h"
#include "../src/Drawables/Mesh.h" 
#include "../Generators/TerrainGenerator.h"
#include "../src/Core/Engine/Application.h"


bool UiContext::init(Window* window) {
    this->window = window;
    if (window == nullptr) {
        std::cerr << "Window is null!" << std::endl;
        return false;
    }

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->getNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Setup ImGui style

    // Check if layout file exists
    std::ifstream iniFile(io.IniFilename);
    if (iniFile.good()) {
        std::cout << "Loading layout from file: " << io.IniFilename << std::endl;
        ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    }
    else {
        std::cout << "No .ini layout file found. Using default layout." << std::endl;
    }

    return true;
}

void UiContext::shutdown() {
    // save layout
    ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UiContext::preRender() {
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UiContext::render() {
    // Render the Menu bar
    renderMenuBar();

    // Render the docking window
    renderDockingWindow();

    // Render the universal buttons (generate, export) window
    renderUniversalButtons();
}

void UiContext::postRender() {
    // ImGui Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void UiContext::setTerrainGenerator(TerrainGenerator* tg) {
    terrainGen = tg;
}

void UiContext::exportTerrain() {
    if (!terrainGen) {
        std::cerr << "TerrainGenerator not set.\n";
        return;
    }

    Mesh* meshData = terrainGen->GetMesh();
    if (meshData) {
        // Safely cast from Mesh* to TerrainMesh*
        TerrainMesh* terrainData = dynamic_cast<TerrainMesh*>(meshData);
        if (terrainData) {

            // Export using base class interface
            ExportMeshAsFBX(*terrainData, "terrain.fbx");
        }
        else {
            std::cerr << "Mesh is not a TerrainMesh.\n";
        }
    }
    else {
        std::cerr << "Terrain mesh not generated.\n";
    }
}


void UiContext::renderDockingWindow() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Create dockspace parent window
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("InvisibleDockSpaceWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_MenuBar
    );
    ImGui::PopStyleVar(3);

    // Create dockspace
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
            defaultLayout();
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    }
    ImGui::End();
}

void UiContext::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                // Handle New action
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Handle Exit action
                
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            if (ImGui::BeginMenu("Layout")) {
                if (ImGui::MenuItem("Save Layout")) {
                    std::string file = Utils::FileDialogs::SaveFile("Save Layout", "Layout\0*.ini\0", window->getNativeWindow());

                    if (!file.empty()) {
                        ImGui::SaveIniSettingsToDisk((file + ".ini").c_str());
                    }
                }
                if (ImGui::MenuItem("Load Layout")) {
                    std::string file = Utils::FileDialogs::OpenFile("Load Layout", "Layout\0*.ini\0", window->getNativeWindow());
                    if (!file.empty()) {
                        ImGui::LoadIniSettingsFromDisk(file.c_str());
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Default Layout")) {
                    ImGui::LoadIniSettingsFromMemory("", 0); // This clears the settings
                    defaultLayout();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void UiContext::defaultLayout() {
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear any previous layout
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    // First split the dockspace into main top and bottom regions
    ImGuiID bottom_main;
    ImGuiID top_main = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.7f, nullptr, &bottom_main);

    // Split the top region into left, center, and right
    ImGuiID top_right;
    ImGuiID top_left = ImGui::DockBuilderSplitNode(top_main, ImGuiDir_Left, 0.25f, nullptr, &top_main);
    top_right = ImGui::DockBuilderSplitNode(top_main, ImGuiDir_Right, 0.25f, nullptr, &top_main);

    // Split the bottom region into left, center, and right
    ImGuiID bottom_right;
    ImGuiID bottom_left = ImGui::DockBuilderSplitNode(bottom_main, ImGuiDir_Left, 0.25f, nullptr, &bottom_main);
    bottom_right = ImGui::DockBuilderSplitNode(bottom_main, ImGuiDir_Right, 0.25f, nullptr, &bottom_main);

    // Now we have:
    // - top_left, top_main (center), top_right
    // - bottom_left, bottom_main (center), bottom_right

    // Dock windows to appropriate regions
    ImGui::DockBuilderDockWindow("Scene View", top_main);  // Main view in center-top

    // Right side panels
    ImGui::DockBuilderDockWindow("Terrain Settings", top_right);
    ImGui::DockBuilderDockWindow("Color Settings", top_right);
    ImGui::DockBuilderDockWindow("Texture Settings", top_right);

    // Left side panels
    // (Add your left-side windows here if needed)

    // Bottom regions
    ImGui::DockBuilderDockWindow("Universal buttons", bottom_left);  // Main bottom area
    // (You can dock other windows to bottom_left or bottom_right as needed)

    ImGui::DockBuilderFinish(dockspace_id);
}

void UiContext::renderUniversalButtons() {
    // Remove user resizing and moving, lock position and size
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove;

    // Get main viewport to position relative to window
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Set window position to bottom-left corner
    ImVec2 windowPos = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - (ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2));
    ImGui::SetNextWindowPos(windowPos);

    // Set fixed window size
    ImGui::SetNextWindowSize(ImVec2(500, ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y * 2));

    // Begin window with updated flags 
    ImGui::Begin("Universal buttons", nullptr, flags);

    // Calculate button width (half the window width minus padding and spacing)
    float windowWidth = ImGui::GetWindowWidth();
    float buttonWidth = (windowWidth - ImGui::GetStyle().WindowPadding.x * 2 - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    // Generate button (left side)
    if (ImGui::Button("Generate", ImVec2(buttonWidth, -1))) {
        // Add your generate functionality here
    }

    ImGui::SameLine();

    // Export button (right side)
    if (ImGui::Button("Export", ImVec2(buttonWidth, -1))) {
        exportTerrain();
    }

    ImGui::End();
}