#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include "SpectrumUI.h"

class Application;

class GlobalButtons {
public:
    void render();

private:
    void renderGenerationButtons();
    void renderExportButton();
};