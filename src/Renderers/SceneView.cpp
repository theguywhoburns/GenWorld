#include <GenWorld/Renderers/SceneView.h>

bool SceneView::init(AppWindow *wind) {
  window = wind;
  m_ViewportSize = window->getViewportSize();

  m_ShadingPanel.setOnParametersChanged(
      [this](const ShadingParameters &params) {
        renderer->updateShadingParameters(params);
      });
  return true;
}

void SceneView::render() {
  processInput();

  renderSceneView();
  renderViewportShading();
  renderGlobalButtons();
}

void SceneView::renderSceneView() {
  ImVec2 min_size(150.0f, 150.0f);
  ImVec2 max_size(INT16_MAX, INT16_MAX);
  ImGui::SetNextWindowSizeConstraints(min_size, max_size);
  ImGui::Begin("Scene View", nullptr,
               ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoBringToFrontOnFocus);
  ImVec2 viewportSize = ImGui::GetContentRegionAvail();

  m_ViewportSize = {viewportSize.x, viewportSize.y};
  renderer->SetScreenSize(m_ViewportSize);
  framebuffer.Resize(m_ViewportSize.x, m_ViewportSize.y);
  window->setViewPortSize(framebuffer.GetWidth(), framebuffer.GetHeight());

  framebuffer.bind();
  window->clearBuffers();
  renderer->Render();
  framebuffer.unbind();

  // add rendered texture to ImGUI scene window
  uint64_t textureID = framebuffer.GetColorTextureID();
  ImGui::Image(textureID, viewportSize, ImVec2{0, 1}, ImVec2{1, 0});

  isSceneWindowHovered =
      ImGui::IsItemHovered() && ImGui::GetIO().WantCaptureMouse;

  ImGui::End();
}

void SceneView::renderViewportShading() { m_ShadingPanel.render(); }

void SceneView::renderGlobalButtons() { m_GlobalButtons.render(); }

#pragma region Callbacks
void SceneView::processInput() {
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

  if (!camMode)
    return;

  camera->processMouseMovement(xoffset, yoffset);
}

void SceneView::mouseClick() {
  // might want to choose IsMouseClicked instead of IsMouseDown
  if (isSceneWindowHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
    camMode = true;
  } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
    camMode = false;
  }
}
#pragma endregion
