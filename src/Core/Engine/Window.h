/*
 * Changes made:
 * 1. Renamed Window class to AppWindow to avoid conflicts with X11 Window typedef
 * 2. Updated all method declarations to use AppWindow instead of Window
 * 3. Updated comment to reflect the new class name
 * 
 * This prevents compilation errors on Linux where X11 defines Window as a typedef,
 * causing conflicts with our Window class definition.
 */

#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Prevent X11 Window typedef from conflicting with our AppWindow class
#ifdef Window
#undef Window
#endif

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

#include "../../Utils/Time.h"

class AppWindow {
public:
    ~AppWindow();

    bool init();
    void shutdown();

    void newFrame();
    void onUpdate();

    void setSize(int width, int height);
    void setSize(const glm::vec2& size);
    void setTitle(const std::string& newTitle) { title = newTitle; }
    glm::vec2 getSize() const { return glm::vec2(SCR_WIDTH, SCR_HEIGHT); }
    void setVsync(bool vsync) { glfwSwapInterval(vsync ? 1 : 0); }

    void setClearColor(const glm::vec4& color) { glClearColor(color.r, color.g, color.b, color.a); }
    void clearBuffers() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); }

    GLFWwindow* getNativeWindow() const { return window; }
    void close() { glfwSetWindowShouldClose(window, true); }
    bool shouldClose() const { return glfwWindowShouldClose(window); }

private:
    unsigned int SCR_WIDTH = 1024;
    unsigned int SCR_HEIGHT = 768;
    std::string title = "GenWorld";
    GLFWwindow* window = nullptr;

    struct MousePosition {
        double x, y;
    };
    MousePosition cursorPos = { 0.0, 0.0 };

    void updateTitle();
    void calculateMousePos();
    void processInput();
    static void resize_callback(GLFWwindow* window, int width, int height);

};
