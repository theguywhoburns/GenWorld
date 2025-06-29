#include "SpectrumUI.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <filesystem>

#define FONT_PATH_REGULAR "Resources/assets/fonts/Source_Sans_3/static/SourceSans3-Regular.ttf"
#define FONT_PATH_BOLD "Resources/assets/fonts/Source_Sans_3/static/SourceSans3-Bold.ttf"

namespace ImGui
{
    namespace Spectrum
    {
        ImFont *RegularFont = nullptr;
        ImFont *BoldFont = nullptr;

        void LoadFonts(float regularSize, float boldSize)
        {
            ImGuiIO &io = ImGui::GetIO();

            // Clear existing fonts if needed
            io.Fonts->Clear();

            // Check if font files exist
            bool regularExists = std::filesystem::exists(FONT_PATH_REGULAR);
            bool boldExists = std::filesystem::exists(FONT_PATH_BOLD);

            if (!regularExists || !boldExists)
            {
                std::cerr << "Warning: Font files not found at expected locations!" << std::endl;
                std::cerr << "Regular font (" << FONT_PATH_REGULAR << "): "
                          << (regularExists ? "Found" : "Not found") << std::endl;
                std::cerr << "Bold font (" << FONT_PATH_BOLD << "): "
                          << (boldExists ? "Found" : "Not found") << std::endl;
                std::cerr << "Using default font instead." << std::endl;

                // Use default font if custom fonts aren't available
                RegularFont = io.Fonts->AddFontDefault();
                BoldFont = RegularFont; // Use regular as bold too
            }
            else
            {
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
        void LoadFont(float size)
        {
            LoadFonts(size, size * 1.3f); // Bold is 20% larger by default
        }

        // Helper function to use title font for a section
        void BeginTitleFont()
        {
            if (BoldFont)
            {
                ImGui::PushFont(BoldFont);
            }
        }

        // Helper function to end title font
        void EndTitleFont()
        {
            if (BoldFont)
            {
                ImGui::PopFont();
            }
        }

        // Convenience wrapper to draw a title with the bold font
        bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags)
        {
            BeginTitleFont();
            bool result = ImGui::CollapsingHeader(label, flags);
            EndTitleFont();
            return result;
        }

        // Convenience wrapper for section titles that aren't collapsing headers
        void SectionTitle(const char *title)
        {
            BeginTitleFont();
            ImGui::Text("%s", title);
            EndTitleFont();
            ImGui::Separator();
            ImGui::Spacing();
        }

        void StyleColorsSpectrum()
        {
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

            // Colors
            ImVec4 *colors = style.Colors;

            // Text colors
            colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(GRAY800);
            colors[ImGuiCol_TextDisabled] = ImGui::ColorConvertU32ToFloat4(GRAY600);

            // Window background
            colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(GRAY100);

            // Headers
            colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(GRAY400);
            colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(BLUE500);

            // Buttons
            colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(GRAY400);
            colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(BLUE500);

            // Frame backgrounds (checkbox, radio button, etc.)
            colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(GRAY200);
            colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(GRAY400);

            // Tabs
            colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(GRAY200);
            colors[ImGuiCol_TabHovered] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_TabActive] = ImGui::ColorConvertU32ToFloat4(GRAY400);
            colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(GRAY200);
            colors[ImGuiCol_TabUnfocusedActive] = ImGui::ColorConvertU32ToFloat4(GRAY300);

            // Title bar
            colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(GRAY200);
            colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(GRAY200);

            // Resize grip
            colors[ImGuiCol_ResizeGrip] = ImGui::ColorConvertU32ToFloat4(GRAY400);
            colors[ImGuiCol_ResizeGripHovered] = ImGui::ColorConvertU32ToFloat4(GRAY600);
            colors[ImGuiCol_ResizeGripActive] = ImGui::ColorConvertU32ToFloat4(GRAY700);

            // Scrollbar
            colors[ImGuiCol_ScrollbarBg] = ImGui::ColorConvertU32ToFloat4(GRAY200);
            colors[ImGuiCol_ScrollbarGrab] = ImGui::ColorConvertU32ToFloat4(GRAY400);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImGui::ColorConvertU32ToFloat4(GRAY500);
            colors[ImGuiCol_ScrollbarGrabActive] = ImGui::ColorConvertU32ToFloat4(GRAY600);

            // Check mark
            colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(BLUE600);

            // Slider
            colors[ImGuiCol_SliderGrab] = ImGui::ColorConvertU32ToFloat4(BLUE400);
            colors[ImGuiCol_SliderGrabActive] = ImGui::ColorConvertU32ToFloat4(BLUE600);

            // Menu bar
            colors[ImGuiCol_MenuBarBg] = ImGui::ColorConvertU32ToFloat4(GRAY100);

            // Separators
            colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_SeparatorHovered] = ImGui::ColorConvertU32ToFloat4(GRAY400);
            colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(GRAY500);

            // Other
            colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(GRAY50);
            colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(GRAY300);
            colors[ImGuiCol_BorderShadow] = ImGui::ColorConvertU32ToFloat4(Static::NONE);
            colors[ImGuiCol_ModalWindowDimBg] = ImGui::ColorConvertU32ToFloat4(color_alpha(180, Static::BLACK));
        }
    }
}
