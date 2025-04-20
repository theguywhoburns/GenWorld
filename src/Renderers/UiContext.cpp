#include "UiContext.h"

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
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                // Handle Open action
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                // Handle Save action
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                // Handle Exit action
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                // Handle Undo action
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                // Handle Redo action
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                // Handle Cut action
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                // Handle Copy action
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                // Handle Paste action
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Scene View")) {
                // Toggle Scene View visibility
            }
            if (ImGui::MenuItem("Properties")) {
                // Toggle Properties visibility
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

    // regions
    ImGuiID top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.2f, nullptr, &dockspace_id);
    ImGuiID down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.25f, nullptr, &dockspace_id);
    ImGuiID left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
    ImGuiID right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);

    ImGui::DockBuilderDockWindow("Scene View", dockspace_id);
    ImGui::DockBuilderDockWindow("Terrain Settings", right);
    ImGui::DockBuilderDockWindow("Color Settings", right);
    ImGui::DockBuilderDockWindow("Texture Settings", right);
    // Add more windows later

    ImGui::DockBuilderFinish(dockspace_id);
}
