#include <ImGuiCurveTest.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <Shader.h>
#include <Camera.h>
#include <Texture.h>
#include <Drawables/Model.h>
#include <Generators/TerrainGenerator.h>
#include <Controllers/TerrainController.h>
#include <Drawables/Lights.h>
#include <Renderer.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

struct MousePosition {
	double x, y;
};

float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

MousePosition cursorPos = { 0.0, 0.0 };

bool camMode = false;
Camera camera(glm::vec3(0.0f, 20.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
float lastX = 0, lastY = 0;

void calculateDeltaTime();
void processInput(GLFWwindow* window);
double calculateFPS(GLFWwindow* window);
void calculateMousePos(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void updateTitle(GLFWwindow* window, const char* title, double fps);
void mouseClick_callback(GLFWwindow* window, int button, int action, int mods);

unsigned int SCR_WIDTH = 1024;
unsigned int SCR_HEIGHT = 768;

int main(void) {
	GLFWwindow* window;
	char title[] = "OpenGL Project";

	/* Initialize the library */
	if (!glfwInit())
		return -1;

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
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// gladLoadGL();
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// window resize callback
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouseClick_callback);

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("Refresh Rate: %dHz\n", glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);

	// Setup ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	Renderer renderer;
	Shader ourShader("Shaders/Terrain.vert", "Shaders/Terrain.frag");
	renderer.SetShader(&ourShader);
	renderer.SetCamera(&camera);
	renderer.SetScreenSize(SCR_WIDTH, SCR_HEIGHT);

	// Plane Data
	TerrainController terrainController(&renderer);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window)) {
		calculateDeltaTime();
		processInput(window);
		calculateMousePos(window);
		updateTitle(window, title, calculateFPS(window));

		renderer.ClearQueue();

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		/* Render here */
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // background color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		terrainController.DisplayUI();
		terrainController.Update();

		renderer.Render();

		// ImGui Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

#pragma region Callbacks
void calculateDeltaTime() {
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (camMode) {
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.processKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.processKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.processKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.processKeyboard(RIGHT, deltaTime);
	}
}

double calculateFPS(GLFWwindow* window) {
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

void calculateMousePos(GLFWwindow* window) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	cursorPos.x = xpos;
	cursorPos.y = ypos;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	float xoffset = static_cast<float>(xpos - lastX);
	float yoffset = static_cast<float>(lastY - ypos);
	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	if (!camMode) return;

	camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.processMouseScroll(static_cast<float>(yoffset));
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
	glViewport(0, 0, width, height);
}

void updateTitle(GLFWwindow* window, const char* title, double fps) {
	char newTitle[256];
	sprintf(newTitle, "%s | FPS: %.1f | Cursor Position: %.0f, %.0f",
		title, fps, cursorPos.x, cursorPos.y);
	glfwSetWindowTitle(window, newTitle);
}

void mouseClick_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		camMode = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		camMode = false;
	}
}
#pragma endregion
