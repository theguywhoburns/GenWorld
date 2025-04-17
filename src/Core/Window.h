#ifndef WINDOW_H
#define WINDOW_H

#include <glfw/glfw3.h>
#include <glad/glad.h>
#include <iostream>
#include "Application.h"


class Window {
private:
    GLFWwindow* window;
    glm::vec2 screenSize;
    std::string title;
    struct MousePosition {
        double x, y;
    };

    MousePosition cursorPos = { 0.0, 0.0 };

public:
    Window(int width, int height, const char* title);
    bool init();
    void shutdown();
    void setTitle(char* title);
    void setVSync(bool enabled);
    void closeWindow();
    void onUpdate();
    bool shouldClose();
    void updateTitle();
    void SetScreenSize(glm::vec2 size);
    void SetScreenSize(float width, float height);
    glm::vec2 GetScreenSize();

    void getCursorPos(double* x, double* y) {
        *x = cursorPos.x;
        *y = cursorPos.y;
    }

    int getHeight() {
        return screenSize.y;
    }

    int getWidth() {
        return screenSize.x;
    }

    void calculateMousePos();
    GLFWwindow* getNativeWindow() { return window; }
};

#endif
