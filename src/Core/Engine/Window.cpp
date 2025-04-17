#include "Window.h"

Window::~Window() {
    shutdown();
}

bool Window::init() {
    if(!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title.c_str(), NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

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

    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, resize_callback);

    return true;
}

void Window::shutdown() {
    // Cleanup resources
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Window::newFrame() {
    processInput();
    calculateMousePos();
    updateTitle();

    setClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    clearBuffers();
}

void Window::onUpdate() {
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Window::setSize(int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void Window::setSize(const glm::vec2& size) {
    SCR_WIDTH = (int)size.x;
    SCR_HEIGHT = (int)size.y;
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

void Window::calculateMousePos() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    cursorPos.x = xpos;
    cursorPos.y = ypos;
}

void Window::updateTitle() {
    char newTitle[256];
    sprintf(newTitle, "%s | FPS: %.1f | Cursor Position: %.0f, %.0f",
        title, Utils::Time::frameRate, cursorPos.x, cursorPos.y);
    glfwSetWindowTitle(window, newTitle);
}

void Window::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        close();
    }

    // if instead you want to use ImGui for input handling.
    // if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    //     close();
    // }
}

void Window::resize_callback(GLFWwindow* window, int width, int height)
{
    Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    win->setSize(width, height);
}
