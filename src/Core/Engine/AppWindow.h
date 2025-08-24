#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Prevent X11 Window typedef from conflicting with our AppWindow class
#ifdef Window
#undef Window
#endif

#include <glad/gl.h>
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

  void setViewPortSize(int width, int height);
  void setViewPortSize(const glm::vec2 &size);
  void setTitle(const std::string &newTitle) { title = newTitle; }
  glm::vec2 getSize() const { return glm::vec2(SCR_WIDTH, SCR_HEIGHT); }
  glm::vec2 getViewportSize() const {
    return glm::vec2(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
  }
  void setVsync(bool vsync) { glfwSwapInterval(vsync ? 1 : 0); }

  void clear() {
    setClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    clearBuffers();
  }
  void setClearColor(const glm::vec4 &color) {
    glClearColor(color.r, color.g, color.b, color.a);
  }
  void clearBuffers() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  }

  GLFWwindow *getNativeWindow() const { return window; }
  void close() { glfwSetWindowShouldClose(window, true); }
  bool shouldClose() const { return glfwWindowShouldClose(window); }

private:
  unsigned int SCR_WIDTH = 1024;
  unsigned int SCR_HEIGHT = 768;
  unsigned int VIEWPORT_WIDTH = 1024;
  unsigned int VIEWPORT_HEIGHT = 768;

  std::string title = "GenWorld";
  GLFWwindow *window = nullptr;

  struct MousePosition {
    double x, y;
  };
  MousePosition cursorPos = {0.0, 0.0};

  void updateTitle();
  void calculateMousePos();
  void processInput();
  static void resize_callback(GLFWwindow *window, int width, int height);
};
