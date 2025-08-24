#include <GenWorld/UI/SpectrumUI.h>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

#define FONT_PATH_REGULAR                                                      \
  "Resources/assets/fonts/Source_Sans_3/static/SourceSans3-Regular.ttf"
#define FONT_PATH_BOLD                                                         \
  "Resources/assets/fonts/Source_Sans_3/static/SourceSans3-Bold.ttf"

namespace ImGui {
namespace Spectrum {
ImFont *RegularFont = nullptr;
ImFont *BoldFont = nullptr;

// Theme state tracking
bool IsLightTheme = false;

void LoadFonts(float regularSize, float boldSize) {
  ImGuiIO &io = ImGui::GetIO();

  // Clear existing fonts if needed
  io.Fonts->Clear();

  // Check if font files exist
  bool regularExists = std::filesystem::exists(FONT_PATH_REGULAR);
  bool boldExists = std::filesystem::exists(FONT_PATH_BOLD);

  if (!regularExists || !boldExists) {
    std::cerr << "Warning: Font files not found at expected locations!"
              << std::endl;
    std::cerr << "Regular font (" << FONT_PATH_REGULAR
              << "): " << (regularExists ? "Found" : "Not found") << std::endl;
    std::cerr << "Bold font (" << FONT_PATH_BOLD
              << "): " << (boldExists ? "Found" : "Not found") << std::endl;
    std::cerr << "Using default font instead." << std::endl;

    // Use default font if custom fonts aren't available
    RegularFont = io.Fonts->AddFontDefault();
    BoldFont = RegularFont; // Use regular as bold too
  } else {
    // Add fonts with different sizes
    RegularFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_REGULAR, regularSize);
    BoldFont = io.Fonts->AddFontFromFileTTF(FONT_PATH_BOLD, boldSize);
  }

  // Build font atlas
  unsigned char *pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  // Set the regular font as the default
  io.FontDefault = RegularFont;
}

// Kept for backward compatibility
void LoadFont(float size) {
  LoadFonts(size, size * 1.3f); // Bold is 20% larger by default
}

// Helper function to use title font for a section
void BeginTitleFont() {
  if (BoldFont) {
    ImGui::PushFont(BoldFont);
  }
}

// Helper function to end title font
void EndTitleFont() {
  if (BoldFont) {
    ImGui::PopFont();
  }
}

// Convenience wrapper to draw a title with the bold font
bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags) {
  BeginTitleFont();
  bool result = ImGui::CollapsingHeader(label, flags);
  EndTitleFont();
  return result;
}

// Convenience wrapper for section titles that aren't collapsing headers
void SectionTitle(const char *title) {
  BeginTitleFont();
  ImGui::Text("%s", title);
  EndTitleFont();
  ImGui::Separator();
  ImGui::Spacing();
}

void StyleColorsSpectrum() {
  IsLightTheme = false; // Mark as dark theme

  ImGuiStyle &style = ImGui::GetStyle();

  // Set up style properties
  style.FrameRounding = 4.0f;
  style.WindowRounding = 6.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;
  style.ChildRounding = 4.0f;
  style.WindowPadding = ImVec2(10, 10);
  style.FramePadding = ImVec2(8, 4);
  style.ItemSpacing = ImVec2(10, 8);
  style.ScrollbarSize = 16.0f;
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

  // Colors (using dark theme)
  ImVec4 *colors = style.Colors;

  // Text colors
  colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY800);
  colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY600);

  // Window background
  colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY100);

  // Headers
  colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_HeaderHovered] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);
  colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Dark::BLUE500);

  // Buttons
  colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_ButtonHovered] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);
  colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(Dark::BLUE500);

  // Frame backgrounds (checkbox, radio button, etc.)
  colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY200);
  colors[ImGuiCol_FrameBgHovered] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_FrameBgActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);

  // Tabs
  colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY200);
  colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);
  colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY200);
  colors[ImGuiCol_TabUnfocusedActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);

  // Title bar
  colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY200);
  colors[ImGuiCol_TitleBgActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_TitleBgCollapsed] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY200);

  // Resize grip
  colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);
  colors[ImGuiCol_ResizeGripHovered] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY600);
  colors[ImGuiCol_ResizeGripActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY700);

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY200);
  colors[ImGuiCol_ScrollbarGrab] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY500);
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY600);

  // Check mark
  colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Dark::BLUE600);

  // Slider
  colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(Dark::BLUE400);
  colors[ImGuiCol_SliderGrabActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::BLUE600);

  // Menu bar
  colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY100);

  // Separators
  colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_SeparatorHovered] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY400);
  colors[ImGuiCol_SeparatorActive] =
      ImGui::ColorConvertU32ToFloat4(Dark::GRAY500);

  // Other
  colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY50);
  colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Dark::GRAY300);
  colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(Static::NONE);
  colors[ImGuiCol_ModalWindowDimBg] =
      ImGui::ColorConvertU32ToFloat4(color_alpha(180, Static::BLACK));
}

void StyleColorsLight() {
  IsLightTheme = true; // Mark as light theme

  ImGuiStyle &style = ImGui::GetStyle();

  // Set up style properties (same as dark theme)
  style.FrameRounding = 4.0f;
  style.WindowRounding = 6.0f;
  style.PopupRounding = 4.0f;
  style.ScrollbarRounding = 4.0f;
  style.GrabRounding = 4.0f;
  style.TabRounding = 4.0f;
  style.ChildRounding = 4.0f;
  style.WindowPadding = ImVec2(10, 10);
  style.FramePadding = ImVec2(8, 4);
  style.ItemSpacing = ImVec2(10, 8);
  style.ScrollbarSize = 16.0f;
  style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

  // Colors (using light theme)
  ImVec4 *colors = style.Colors;

  // Text colors
  colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Light::GRAY800);
  colors[ImGuiCol_TextDisabled] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY600);

  // Window background
  colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Light::GRAY100);

  // Headers
  colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_HeaderHovered] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY400);
  colors[ImGuiCol_HeaderActive] =
      ImGui::ColorConvertU32ToFloat4(Light::BLUE500);

  // Buttons
  colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_ButtonHovered] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY400);
  colors[ImGuiCol_ButtonActive] =
      ImGui::ColorConvertU32ToFloat4(Light::BLUE500);

  // Frame backgrounds (checkbox, radio button, etc.)
  colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Light::GRAY200);
  colors[ImGuiCol_FrameBgHovered] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_FrameBgActive] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY400);

  // Tabs
  colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Light::GRAY200);
  colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(Light::GRAY400);
  colors[ImGuiCol_TabUnfocused] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY200);
  colors[ImGuiCol_TabUnfocusedActive] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY300);

  // Title bar
  colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Light::GRAY200);
  colors[ImGuiCol_TitleBgActive] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_TitleBgCollapsed] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY200);

  // Resize grip
  colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(Light::GRAY400);
  colors[ImGuiCol_ResizeGripHovered] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY600);
  colors[ImGuiCol_ResizeGripActive] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY700);

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(Light::GRAY200);
  colors[ImGuiCol_ScrollbarGrab] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY400);
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY500);
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY600);

  // Check mark
  colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Light::BLUE600);

  // Slider
  colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(Light::BLUE400);
  colors[ImGuiCol_SliderGrabActive] =
      ImGui::ColorConvertU32ToFloat4(Light::BLUE600);

  // Menu bar
  colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(Light::GRAY100);

  // Separators
  colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_SeparatorHovered] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY400);
  colors[ImGuiCol_SeparatorActive] =
      ImGui::ColorConvertU32ToFloat4(Light::GRAY500);

  // Other
  colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Light::GRAY50);
  colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Light::GRAY300);
  colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(Static::NONE);
  colors[ImGuiCol_ModalWindowDimBg] =
      ImGui::ColorConvertU32ToFloat4(color_alpha(180, Static::BLACK));
}

// Runtime color getters that return appropriate colors based on current theme
unsigned int GRAY50() { return IsLightTheme ? Light::GRAY50 : Dark::GRAY50; }
unsigned int GRAY75() { return IsLightTheme ? Light::GRAY75 : Dark::GRAY75; }
unsigned int GRAY100() { return IsLightTheme ? Light::GRAY100 : Dark::GRAY100; }
unsigned int GRAY200() { return IsLightTheme ? Light::GRAY200 : Dark::GRAY200; }
unsigned int GRAY300() { return IsLightTheme ? Light::GRAY300 : Dark::GRAY300; }
unsigned int GRAY400() { return IsLightTheme ? Light::GRAY400 : Dark::GRAY400; }
unsigned int GRAY500() { return IsLightTheme ? Light::GRAY500 : Dark::GRAY500; }
unsigned int GRAY600() { return IsLightTheme ? Light::GRAY600 : Dark::GRAY600; }
unsigned int GRAY700() { return IsLightTheme ? Light::GRAY700 : Dark::GRAY700; }
unsigned int GRAY800() { return IsLightTheme ? Light::GRAY800 : Dark::GRAY800; }
unsigned int GRAY900() { return IsLightTheme ? Light::GRAY900 : Dark::GRAY900; }
unsigned int BLUE400() { return IsLightTheme ? Light::BLUE400 : Dark::BLUE400; }
unsigned int BLUE500() { return IsLightTheme ? Light::BLUE500 : Dark::BLUE500; }
unsigned int BLUE600() { return IsLightTheme ? Light::BLUE600 : Dark::BLUE600; }
unsigned int BLUE700() { return IsLightTheme ? Light::BLUE700 : Dark::BLUE700; }
} // namespace Spectrum
} // namespace ImGui
