#pragma once

#include "IRenderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

class UiRenderer : public IRenderer {
public:
    UiRenderer(GLFWwindow* window) : window(window) {}
    ~UiRenderer() override = default;

    void init() override;
    void shutdown() override;

    void preRender() override;
    void render() override;
    void postRender() override;

private:
    void renderDockingWindow();
    void renderTitleBar();

    GLFWwindow* window = nullptr;

};