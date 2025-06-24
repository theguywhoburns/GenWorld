/*
 * Changes made:
 * 1. Updated all references of Window to AppWindow in method signatures and inheritance
 * 
 * This ensures consistency with the renamed Window class to AppWindow to avoid
 * X11 Window typedef conflicts on Linux.
 */

#pragma once

#include "IRenderContext.h"
#include "../Utils/FileDialogs.h"
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

private:
    void renderDockingWindow();
    void renderMenuBar();
    void defaultLayout();

};