#pragma once

#include "IRenderContext.h"
#include <GLFW/glfw3.h>

#include <iostream>

class UiContext : public IRenderContext {
public:
    ~UiContext() override = default;

    bool init(GLFWwindow* window) override;
    void shutdown() override;

    void preRender() override;
    void render() override;
    void postRender() override;

private:
    void renderDockingWindow();
    void renderTitleBar();

};