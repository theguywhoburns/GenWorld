
#include <GenWorld/Core/Engine/AppWindow.h>
#include <GenWorld/Core/stb_image.h>
#include <GenWorld/Utils/OpenGlInc.h>
AppWindow::~AppWindow() { shutdown(); }

bool AppWindow::init() {
  if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW" << std::endl;
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Start in fullscreen mode
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title.c_str(), NULL, NULL);

  if (!window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return false;
  }

  glfwMaximizeWindow(window);
  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return false;
  }

  // Set Icon
  int width, height, nrChannels;
  unsigned char *data =
      stbi_load("Resources/Icon.png", &width, &height, &nrChannels, 4);

  if (data) {
    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = data;
    glfwSetWindowIcon(window, 1, &image);
  } else {
    std::cout << "Failed to load icon" << std::endl;
  }
  stbi_image_free(data);

  printf("OpenGL version: %s\n", glGetString(GL_VERSION));
  printf("Refresh Rate: %dHz\n",
         glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);

  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
  glEnable(GL_CULL_FACE); // Enable face culling
  glCullFace(GL_BACK);    // Cull back faces (default)
  glFrontFace(GL_CCW);    // Set front face to counter-clockwise winding order
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // uncomment this call to draw in wireframe polygons.
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glfwSetWindowUserPointer(window, this);
  glfwSetWindowSizeCallback(window, resize_callback);

  return true;
}

void AppWindow::shutdown() {
  // Cleanup resources
  glfwDestroyWindow(window);
  glfwTerminate();
}

void AppWindow::newFrame() {
  processInput();
  calculateMousePos();
  updateTitle();
}

void AppWindow::onUpdate() {
  // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved
  // etc.)
  // -------------------------------------------------------------------------------
  glfwSwapBuffers(window);
  glfwPollEvents();
}

void AppWindow::setViewPortSize(int width, int height) {
  VIEWPORT_WIDTH = width;
  VIEWPORT_HEIGHT = height;
  glViewport(0, 0, width, height);
}

void AppWindow::setViewPortSize(const glm::vec2 &size) {
  VIEWPORT_WIDTH = (int)size.x;
  VIEWPORT_HEIGHT = (int)size.y;
  glViewport(0, 0, size.x, size.y);
}

void AppWindow::calculateMousePos() {
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  cursorPos.x = xpos;
  cursorPos.y = ypos;
}

void AppWindow::updateTitle() {
  char newTitle[256];
  sprintf(newTitle, "%s | FPS: %.1f | Cursor Position: %.0f, %.0f",
          title.c_str(), Utils::Time::frameRate, cursorPos.x, cursorPos.y);
  glfwSetWindowTitle(window, newTitle);
}

void AppWindow::processInput() {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    close();
  }

  // if instead you want to use ImGui for input handling.
  // if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
  //     close();
  // }
}

void AppWindow::resize_callback(GLFWwindow *window, int width, int height) {
  AppWindow *win = static_cast<AppWindow *>(glfwGetWindowUserPointer(window));
  win->SCR_WIDTH = width;
  win->SCR_HEIGHT = height;
}
void AppWindow::setClearColor(const glm::vec4 &color) {
  glClearColor(color.r, color.g, color.b, color.a);
}

void AppWindow::clearBuffers() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}