#include "Window.h"


Window::Window(int width, int height, const char* title)
{
    this->screenSize.x = width;
    this->screenSize.y = height;

    this->window = nullptr;
    this->title = title;
}

void Window::shutdown()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::init()
{
            // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
   
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(screenSize.y, screenSize.x, title.c_str(), NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    // Vsync
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));
    printf("Refresh Rate: %dHz\n", glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    return true;
}


void Window::onUpdate()
{
    glfwSwapBuffers(window);
    glfwPollEvents();
}


void Window::updateTitle()
{
    char newTitle[256];
    sprintf(newTitle, "%s | FPS: %.1f | Cursor Position: %.0f, %.0f",
    title, Utils::Time::frameRate, cursorPos.x, cursorPos.y);
    glfwSetWindowTitle(window, newTitle);
}

void Window::setTitle(char *title)
{
    this->title = title;
}


void Window::setVSync(bool enabled)
{
    glfwSwapInterval(enabled);
}

bool Window::shouldClose()
{
    return glfwWindowShouldClose(window);
}

void Window::closeWindow() {
    glfwSetWindowShouldClose(window, true);
}

void Window::SetScreenSize(glm::vec2 size) {
    screenSize = size;
}

void Window::SetScreenSize(float width, float height) {
    screenSize = glm::vec2(width, height);
}

glm::vec2 Window::GetScreenSize() {
    return screenSize;
}

void Window::calculateMousePos()
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    cursorPos.x = xpos;
    cursorPos.y = ypos;
}

