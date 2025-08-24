#include <GenWorld/Core/Engine/Application.h>
#include <GenWorld/UI/GlobalButtons.h>

void GlobalButtons::render() {
  const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
                      ImGuiWindowFlags_NoSavedSettings |
                      ImGuiWindowFlags_NoBackground |
                      ImGuiWindowFlags_NoDocking;

  ImGuiWindow *sceneWindow = ImGui::FindWindowByName("Scene View");
  if (!sceneWindow)
    return;

  ImGui::SetNextWindowSize(sceneWindow->Size);
  ImGui::SetNextWindowPos(sceneWindow->Pos);
  ImGui::SetNextWindowBgAlpha(0.0f);
  ImGui::SetNextWindowViewport(sceneWindow->ViewportId);

  ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
  ImGui::PushStyleColor(ImGuiCol_Border, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

  ImGui::Begin("GlobalButtonsOverlay", nullptr, flags);

  ImVec2 originalPos = ImGui::GetCursorPos();
  ImVec2 contentRegionMin = ImGui::GetWindowContentRegionMin();
  ImVec2 contentRegionMax = ImGui::GetWindowContentRegionMax();

  // Bottom left - Generate/Randomize seed buttons
  ImVec2 seedButtonsSize(408, 40);
  ImVec2 seedButtonsPos(contentRegionMin.x + 20,
                        contentRegionMax.y - seedButtonsSize.y - 20);

  ImGui::SetCursorPos(seedButtonsPos);

  ImGui::BeginChild(
      "SeedButtons", seedButtonsSize, ImGuiChildFlags_Borders,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
          ImGuiWindowFlags_NoTitleBar);

  renderGenerationButtons();
  ImGui::EndChild();

  // Bottom right - Export button
  ImVec2 exportButtonSize(200, 40);
  ImVec2 exportButtonPos(contentRegionMax.x - exportButtonSize.x - 20,
                         contentRegionMax.y - exportButtonSize.y - 20);

  ImGui::SetCursorPos(exportButtonPos);

  ImGui::BeginChild(
      "ExportButton", exportButtonSize, ImGuiChildFlags_Borders,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground |
          ImGuiWindowFlags_NoTitleBar);

  renderExportButton();
  ImGui::EndChild();

  ImGui::SetCursorPos(originalPos);
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void GlobalButtons::renderGenerationButtons() {
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));

  // Generate button
  ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(
                                             ImGui::Spectrum::BLUE500()));
  ImGui::PushStyleColor(
      ImGuiCol_ButtonHovered,
      ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::BLUE600()));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(
                                                   ImGui::Spectrum::BLUE400()));

  if (ImGui::Button("Generate", ImVec2(200, 40))) {
    Application::GetInstance().Generate();
  }

  ImGui::PopStyleColor(3);

  ImGui::SameLine();

  // Randomize button
  ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(
                                             ImGui::Spectrum::GRAY400()));
  ImGui::PushStyleColor(
      ImGuiCol_ButtonHovered,
      ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::GRAY500()));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::ColorConvertU32ToFloat4(
                                                   ImGui::Spectrum::GRAY300()));

  if (ImGui::Button("Randomize", ImVec2(200, 40))) {
    Application::GetInstance().RandomizeSeed();
  }

  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar();
}

void GlobalButtons::renderExportButton() {
  // Export button
  ImGui::PushStyleColor(
      ImGuiCol_Button,
      ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GREEN500));
  ImGui::PushStyleColor(
      ImGuiCol_ButtonHovered,
      ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GREEN600));
  ImGui::PushStyleColor(
      ImGuiCol_ButtonActive,
      ImGui::ColorConvertU32ToFloat4(ImGui::Spectrum::Static::GREEN400));

  if (ImGui::Button("Export", ImVec2(200, 40))) {
    // TODO: need to show a submenu to select the export format first
    Application::GetInstance().Export("obj");
  }

  ImGui::PopStyleColor(3);
}