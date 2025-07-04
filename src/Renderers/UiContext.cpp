#include "UiContext.h"
#include "../Utils/ImGuizmo.h"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

#include "../src/Core/Engine/Application.h"


void UiContext::switchTheme()
{
    isDarkTheme = !isDarkTheme;
    if (isDarkTheme)
    {
        ImGui::Spectrum::StyleColorsSpectrum();
    }
    else
    {
        ImGui::Spectrum::StyleColorsLight();
    }
}

bool UiContext::init(AppWindow *window)
{

    this->window = window;
    if (window == nullptr)
    {
        std::cerr << "Window is null!" << std::endl;
        return false;
    }

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->getNativeWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Initialize with dark theme by default
    ImGui::Spectrum::StyleColorsSpectrum();
    ImGui::Spectrum::LoadFonts(25.0f, 30.0f); // Load regular font at 25pt, bold font at 30pt

    // Check if layout file exists
    std::ifstream iniFile(io.IniFilename);
    if (iniFile.good())
    {
        std::cout << "Loading layout from file: " << io.IniFilename << std::endl;
        ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    }
    else
    {
        std::cout << "No .ini layout file found. Using default layout." << std::endl;
    }

    return true;
}

void UiContext::shutdown()
{
    // save layout
    ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UiContext::preRender()
{
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UiContext::render()
{
    // Render the Menu bar
    renderMenuBar();


    // Render the docking window
    renderDockingWindow();

    // Render the scene overlay
    renderSceneOverlay();
}

void UiContext::postRender()
{
    // ImGui Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

void UiContext::exportMesh(std::string format) {
    Application::GetInstance()->Export(format);
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
                     ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar(3);

    // Create dockspace
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
        {
            defaultLayout();
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoWindowMenuButton);
    }
    ImGui::End();
}

void UiContext::renderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::BeginMenu("Export")) {
                if (ImGui::MenuItem("OBJ", "(.obj + .mtl)")) {
                    exportMesh("obj");
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
               Application::GetInstance()->Quit();
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("View")) {
            // Theme selector as a submenu item
            if (ImGui::BeginMenu("Theme"))
            {
                if (ImGui::MenuItem("Dark Theme", nullptr, isDarkTheme))
                {
                    if (!isDarkTheme)
                        switchTheme();
                }
                if (ImGui::MenuItem("Light Theme", nullptr, !isDarkTheme))
                {
                    if (isDarkTheme)
                        switchTheme();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::BeginMenu("Layout"))
            {
                if (ImGui::MenuItem("Save Layout"))
                {
                    std::string file = Utils::FileDialogs::SaveFile("Save Layout", "Layout\0*.ini\0", window->getNativeWindow());

                    if (!file.empty())
                    {
                        ImGui::SaveIniSettingsToDisk((file + ".ini").c_str());
                    }
                }
                if (ImGui::MenuItem("Load Layout"))
                {
                    std::string file = Utils::FileDialogs::OpenFile("Load Layout", "Layout\0*.ini\0", window->getNativeWindow());
                    if (!file.empty())
                    {
                        ImGui::LoadIniSettingsFromDisk(file.c_str());
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Default Layout"))
                {
                    ImGui::LoadIniSettingsFromMemory("", 0); // This clears the settings
                    defaultLayout();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        
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
}

void UiContext::renderSceneOverlay() {
    glm::mat4 view = camera->GetViewMatrix();
    float viewMatrix[16];
    std::memcpy(viewMatrix, glm::value_ptr(view), sizeof(float) * 16);

    glm::vec3 target = camera->position + camera->front * 10.0f;
    float cameraDistance = glm::length(camera->position - target);

    renderSceneOverlay(viewMatrix, cameraDistance);
}

void UiContext::renderSceneOverlay(float* viewMatrix, float cameraDistance) {
    const ImU32 flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground;

    // Try to set the current window to "Scene View" for correct positioning
    ImGuiWindow* sceneWindow = ImGui::FindWindowByName("Scene View");
    if (!sceneWindow) return; // Scene View not found, don't render gizmo

    ImGui::SetNextWindowSize(sceneWindow->Size);
    ImGui::SetNextWindowPos(sceneWindow->Pos);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowViewport(sceneWindow->ViewportId);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::PushStyleColor(ImGuiCol_Border, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGui::Begin("gizmo", NULL, flags);

    ImVec2 scenePos = sceneWindow->Pos;
    ImVec2 sceneSize = sceneWindow->Size;

    // Place gizmo in the top-right corner of the Scene View window
    ImVec2 gizmoSize(150, 150);
    ImVec2 gizmoPos(
        scenePos.x + sceneSize.x - gizmoSize.x - 50, // 30px padding from right
        scenePos.y + 50                              // 30px padding from top
    );

    ImGuizmo::DrawAxisTripod(
        viewMatrix,
        nullptr,
        gizmoPos,
        gizmoSize
    );

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void UiContext::defaultLayout()
{
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

    ImGui::DockBuilderRemoveNode(dockspace_id); // Clear any previous layout
    ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

    // regions
    ImGuiID top = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Up, 0.2f, nullptr, &dockspace_id);
    ImGuiID down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.2f, nullptr, &dockspace_id);
    ImGuiID left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.35f, nullptr, &dockspace_id);
    ImGuiID right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.5f, nullptr, &dockspace_id);

    ImGui::DockBuilderDockWindow("Scene View", dockspace_id);
    ImGui::DockBuilderDockWindow("Terrain Settings", right);
    // Add more windows later

    ImGui::DockBuilderFinish(dockspace_id);
}