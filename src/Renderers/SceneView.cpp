#include "SceneView.h"



void SceneView::preRender() {
    processInput();
    
    int width, height;

    width = window->getWidth();
    height = window->getHeight();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glViewport(0, 0, width, height);
    window->SetScreenSize(width, height);
}


void SceneView::render() {
    ImVec2 min_size(150.0f, 150.0f);
    ImVec2 max_size(INT16_MAX, INT16_MAX);
    ImGui::SetNextWindowSizeConstraints(min_size, max_size);

    ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoCollapse);
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    window->SetScreenSize(viewportSize.x, viewportSize.y);

    framebuffer.Resize(window->getWidth(), window->getHeight());
    framebuffer.bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    renderer->Render();

    framebuffer.unbind();

    // add rendered texture to ImGUI scene window
    uint64_t textureID = framebuffer.GetColorTextureID();
    ImGui::Image(textureID, viewportSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

    isSceneWindowHovered = ImGui::IsItemHovered() && ImGui::GetIO().WantCaptureMouse;

    ImGui::End();
}

#pragma region Callbacks
void SceneView::processInput() {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        window->closeWindow();
    }

    mouseClick();
    mouse_pos_calc();

    if (camMode) {
        if (ImGui::IsKeyDown(ImGuiKey_W))
            camera->processKeyboard(FORWARD, Utils::Time::deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_S))
            camera->processKeyboard(BACKWARD, Utils::Time::deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_A))
            camera->processKeyboard(LEFT, Utils::Time::deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_D))
            camera->processKeyboard(RIGHT, Utils::Time::deltaTime);
    }
}


void SceneView::mouse_pos_calc() {
    ImVec2 mousePos = ImGui::GetMousePos();
    double xpos = mousePos.x;
    double ypos = mousePos.y;
    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    if (!camMode) return;

    camera->processMouseMovement(xoffset, yoffset);
}

void SceneView::mouseClick() {
    // might want to choose IsMouseClicked instead of IsMouseDown
    if (isSceneWindowHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        camMode = true;
    }
    else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        camMode = false;
    }
}
#pragma endregion
