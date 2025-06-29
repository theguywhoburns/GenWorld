#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
#include "../UI/SpectrumUI.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>

class UiContext : public IRenderContext {
public:
    ~UiContext() override = default;

    bool init(AppWindow* window) override;
    void shutdown() override;

    void preRender() override;
    void render() override;
    void postRender() override;

    // Theme functions
    void switchTheme();

private:
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();

    // Theme state
    bool isDarkTheme = true;
};
