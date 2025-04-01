#include "SceneContext.h"

bool SceneContext::init(GLFWwindow* window) {
    this->window = window;
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
    this->window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, title, NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(this->window);

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
    /* Render here */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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
    framebuffer.Resize((int)viewportSize.x, (int)viewportSize.y);
    framebuffer.bind();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

    renderer->SetScreenSize(viewportSize.x, viewportSize.y);
    renderer->Render();

    framebuffer.unbind();

    // add rendered texture to ImGUI scene window
    uint64_t textureID = framebuffer.GetColorTextureID();
    ImGui::Image(textureID, ImVec2{ viewportSize.x, viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    ImGui::End();
}

