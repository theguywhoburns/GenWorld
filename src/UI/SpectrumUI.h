#pragma once

// must be defined before imgui.h
#define IMGUI_DEFINE_MATH_OPERATORS

#include <imgui.h>
/*
Color definitions in ImGui are a good starting point,
but do not cover all the intricacies of Spectrum's possible colors
in controls and widgets.

One big difference is that ImGui communicates widget activity
(hover, pressed) with their background, while spectrum uses a mix
of background and border, with border being the most common choice.

Because of this, we reference extra colors in spectrum from
imgui.cpp and imgui_widgets.cpp directly, and to make that work,
we need to have them defined at here at compile time.
*/

namespace ImGui
{
    namespace Spectrum
    {
        // Font pointers for access throughout the application
        extern ImFont *RegularFont;
        extern ImFont *BoldFont;

        // Loads both regular and bold fonts with different sizes
        void LoadFonts(float regularSize = 19.0f, float boldSize = 25.0f);

        // Helper functions for working with bold fonts
        void BeginTitleFont();
        void EndTitleFont();
        bool CollapsingHeader(const char *label, ImGuiTreeNodeFlags flags = 0);
        void SectionTitle(const char *title);

        // Theme functions
        void StyleColorsSpectrum();  // Dark theme
        void StyleColorsLight();     // Light theme

        namespace
        {
            unsigned int Color(unsigned int c)
            {
                const short a = 0xFF;
                const short r = (c >> 16) & 0xFF;
                const short g = (c >> 8) & 0xFF;
                const short b = (c >> 0) & 0xFF;
                return (a << 24) | (r << 0) | (g << 8) | (b << 16);
            }
        }
        // all colors are from http://spectrum.corp.adobe.com/color.html

        inline unsigned int color_alpha(unsigned int alpha, unsigned int c)
        {
            return ((alpha & 0xFF) << 24) | (c & 0x00FFFFFF);
        }

        namespace Static
        {                                         // static colors
            const unsigned int NONE = 0x00000000; // transparent
            const unsigned int WHITE = Color(0xFFFFFF);
            const unsigned int BLACK = Color(0x000000);
            const unsigned int GRAY200 = Color(0xF4F4F4);
            const unsigned int GRAY300 = Color(0xEAEAEA);
            const unsigned int GRAY400 = Color(0xD3D3D3);
            const unsigned int GRAY500 = Color(0xBCBCBC);
            const unsigned int GRAY600 = Color(0x959595);
            const unsigned int GRAY700 = Color(0x767676);
            const unsigned int GRAY800 = Color(0x505050);
            const unsigned int GRAY900 = Color(0x323232);
            const unsigned int BLUE400 = Color(0x378EF0);
            const unsigned int BLUE500 = Color(0x2680EB);
            const unsigned int BLUE600 = Color(0x1473E6);
            const unsigned int BLUE700 = Color(0x0D66D0);
            const unsigned int RED400 = Color(0xEC5B62);
            const unsigned int RED500 = Color(0xE34850);
            const unsigned int RED600 = Color(0xD7373F);
            const unsigned int RED700 = Color(0xC9252D);
            const unsigned int ORANGE400 = Color(0xF29423);
            const unsigned int ORANGE500 = Color(0xE68619);
            const unsigned int ORANGE600 = Color(0xDA7B11);
            const unsigned int ORANGE700 = Color(0xCB6F10);
            const unsigned int GREEN400 = Color(0x33AB84);
            const unsigned int GREEN500 = Color(0x2D9D78);
            const unsigned int GREEN600 = Color(0x268E6C);
            const unsigned int GREEN700 = Color(0x12805C);
        }

        // Light theme colors
        namespace Light
        {
        const unsigned int GRAY50 = Color(0xFFFFFF);
        const unsigned int GRAY75 = Color(0xFAFAFA);
        const unsigned int GRAY100 = Color(0xF5F5F5);
        const unsigned int GRAY200 = Color(0xEAEAEA);
        const unsigned int GRAY300 = Color(0xE1E1E1);
        const unsigned int GRAY400 = Color(0xCACACA);
        const unsigned int GRAY500 = Color(0xB3B3B3);
        const unsigned int GRAY600 = Color(0x8E8E8E);
        const unsigned int GRAY700 = Color(0x707070);
        const unsigned int GRAY800 = Color(0x4B4B4B);
        const unsigned int GRAY900 = Color(0x2C2C2C);
        const unsigned int BLUE400 = Color(0x2680EB);
        const unsigned int BLUE500 = Color(0x1473E6);
        const unsigned int BLUE600 = Color(0x0D66D0);
        const unsigned int BLUE700 = Color(0x095ABA);
        const unsigned int RED400 = Color(0xE34850);
        const unsigned int RED500 = Color(0xD7373F);
        const unsigned int RED600 = Color(0xC9252D);
        const unsigned int RED700 = Color(0xBB121A);
        const unsigned int ORANGE400 = Color(0xE68619);
        const unsigned int ORANGE500 = Color(0xDA7B11);
        const unsigned int ORANGE600 = Color(0xCB6F10);
        const unsigned int ORANGE700 = Color(0xBD640D);
        const unsigned int GREEN400 = Color(0x2D9D78);
        const unsigned int GREEN500 = Color(0x268E6C);
        const unsigned int GREEN600 = Color(0x12805C);
        const unsigned int GREEN700 = Color(0x107154);
        }

        // Dark theme colors
        namespace Dark
        {
        const unsigned int GRAY50 = Color(0x252525);
        const unsigned int GRAY75 = Color(0x2F2F2F);
        const unsigned int GRAY100 = Color(0x323232);
        const unsigned int GRAY200 = Color(0x393939);
        const unsigned int GRAY300 = Color(0x3E3E3E);
        const unsigned int GRAY400 = Color(0x4D4D4D);
        const unsigned int GRAY500 = Color(0x5C5C5C);
        const unsigned int GRAY600 = Color(0x7B7B7B);
        const unsigned int GRAY700 = Color(0x999999);
        const unsigned int GRAY800 = Color(0xCDCDCD);
        const unsigned int GRAY900 = Color(0xFFFFFF);
        const unsigned int BLUE400 = Color(0x2680EB);
        const unsigned int BLUE500 = Color(0x378EF0);
        const unsigned int BLUE600 = Color(0x4B9CF5);
        const unsigned int BLUE700 = Color(0x5AA9FA);
        const unsigned int RED400 = Color(0xE34850);
        const unsigned int RED500 = Color(0xEC5B62);
        const unsigned int RED600 = Color(0xF76D74);
        const unsigned int RED700 = Color(0xFF7B82);
        const unsigned int ORANGE400 = Color(0xE68619);
        const unsigned int ORANGE500 = Color(0xF29423);
        const unsigned int ORANGE600 = Color(0xF9A43F);
        const unsigned int ORANGE700 = Color(0xFFB55B);
        const unsigned int GREEN400 = Color(0x2D9D78);
        const unsigned int GREEN500 = Color(0x33AB84);
        const unsigned int GREEN600 = Color(0x39B990);
        const unsigned int GREEN700 = Color(0x3FC89C);
        }

        // Runtime-aware color getters - these return the right color based on current theme
        unsigned int GRAY50();
        unsigned int GRAY75();
        unsigned int GRAY100();
        unsigned int GRAY200();
        unsigned int GRAY300();
        unsigned int GRAY400();
        unsigned int GRAY500();
        unsigned int GRAY600();
        unsigned int GRAY700();
        unsigned int GRAY800();
        unsigned int GRAY900();
        unsigned int BLUE400();
        unsigned int BLUE500();
        unsigned int BLUE600();
        unsigned int BLUE700();
    }
}
