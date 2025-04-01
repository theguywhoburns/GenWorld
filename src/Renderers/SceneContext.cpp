#include "SceneContext.h"

bool SceneContext::init(GLFWwindow* wind) {
    char title[] = "GenWorld";

    /* Initialize the library */
    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title, NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // gladLoadGL();
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

void SceneContext::shutdown() {
    // Cleanup resources
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SceneContext::preRender() {
    calculateDeltaTime();
    processInput();
    calculateMousePos();
    updateTitle(calculateFPS());

    /* Render here */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    renderer->SetScreenSize(SCR_WIDTH, SCR_HEIGHT);
}

void SceneContext::postRender() {
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void SceneContext::render() {
    ImGui::Begin("Scene View");
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    SCR_WIDTH = (int)viewportSize.x;
    SCR_HEIGHT = (int)viewportSize.y;

    framebuffer.Resize(SCR_WIDTH, SCR_HEIGHT);
    framebuffer.bind();

    // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    renderer->Render();

    framebuffer.unbind();

    // add rendered texture to ImGUI scene window
    uint64_t textureID = framebuffer.GetColorTextureID();
    ImGui::Image(textureID, viewportSize, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

    isSceneWindowHovered = ImGui::IsItemHovered() && ImGui::GetIO().WantCaptureMouse;

    ImGui::End();
}

#pragma region Callbacks
void SceneContext::calculateDeltaTime() {
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}

void SceneContext::processInput() {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        glfwSetWindowShouldClose(window, true);
    }

    mouseClick();
    mouse_pos_calc();

    if (camMode) {
        if (ImGui::IsKeyDown(ImGuiKey_W))
            camera->processKeyboard(FORWARD, deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_S))
            camera->processKeyboard(BACKWARD, deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_A))
            camera->processKeyboard(LEFT, deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_D))
            camera->processKeyboard(RIGHT, deltaTime);
    }
}

double SceneContext::calculateFPS() {
    static double fps;
    static double previousTime = glfwGetTime();
    static int frameCount = 0;
    double currentTime = glfwGetTime();
    frameCount++;

    if (currentTime - previousTime >= 1.0) {
        fps = frameCount / (currentTime - previousTime);
        frameCount = 0;
        previousTime = currentTime;
    }

    return fps;
}

void SceneContext::calculateMousePos() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    cursorPos.x = xpos;
    cursorPos.y = ypos;
}

void SceneContext::updateTitle(double fps) {
    char newTitle[256];
    sprintf(newTitle, "%s | FPS: %.1f | Cursor Position: %.0f, %.0f",
        title, fps, cursorPos.x, cursorPos.y);
    glfwSetWindowTitle(window, newTitle);
}

void SceneContext::mouse_pos_calc() {
    ImVec2 mousePos = ImGui::GetMousePos();
    double xpos = mousePos.x;
    double ypos = mousePos.y;
    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos;
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    if (!camMode) return;

    camera->processMouseMovement(xoffset, yoffset);
}

void SceneContext::mouseClick() {
    if (isSceneWindowHovered && ImGui::IsMouseDown(1)) {
        camMode = true;
    }
    else if (ImGui::IsMouseReleased(1)) {
        camMode = false;
    }
}
#pragma endregion
