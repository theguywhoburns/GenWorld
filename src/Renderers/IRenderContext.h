#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include "../Core/Application.h"

class IRenderContext {
public:
    IRenderContext() {
        window = Application::getInstance()->getWindow();
    }
    virtual ~IRenderContext() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual void preRender() = 0;
    virtual void render() = 0;
    virtual void postRender() = 0;
protected:
    Window* window;
};
