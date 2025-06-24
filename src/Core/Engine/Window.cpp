/*
 * Changes made:
 * 1. Renamed Window class to AppWindow to avoid conflicts with X11 Window typedef
 * 2. Updated all method implementations to use AppWindow instead of Window
 * 3. Updated the static callback function to cast to AppWindow* instead of Window*
 * 
 * This prevents compilation errors on Linux where X11 defines Window as a typedef,
 * causing conflicts with our Window class definition.
 */

#include "Window.h"
#include "../stb_image.h"

AppWindow::~AppWindow() {
    shutdown();
}

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

    // Set Icon
    int width, height, nrChannels;
    unsigned char* data = stbi_load("Resources/Icon.png", &width, &height, &nrChannels, 4);

    if (data) {
        GLFWimage image;
        image.width = width;
        image.height = height;
        image.pixels = data;
        glfwSetWindowIcon(window, 1, &image);
    }
    else {
        std::cout << "Failed to load icon" << std::endl;
    }
    stbi_image_free(data);  


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

void AppWindow::shutdown() {
    // Cleanup resources
    glfwDestroyWindow(window);
    glfwTerminate();
}

void AppWindow::newFrame() {
    processInput();
    calculateMousePos();
    updateTitle();

    setClearColor(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
    clearBuffers();
}

void AppWindow::onUpdate() {
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void AppWindow::setSize(int width, int height) {
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void AppWindow::setSize(const glm::vec2& size) {
    SCR_WIDTH = (int)size.x;
    SCR_HEIGHT = (int)size.y;
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
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

void AppWindow::resize_callback(GLFWwindow* window, int width, int height) {
    AppWindow* win = static_cast<AppWindow*>(glfwGetWindowUserPointer(window));
    win->setSize(width, height);
}
