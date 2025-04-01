#define IMGUI_DEFINE_MATH_OPERATORS
#define GLFW_INCLUDE_NONE
#include "Renderers/UiContext.h"
#include "Renderers/SceneContext.h"
#include "Renderers/Renderer.h"

#include "Core/Shader.h"
#include "Core/Camera.h"
#include "Core/Texture.h"
#include "Core/Framebuffer.h"
#include "Drawables/Model.h"
#include "Generators/TerrainGenerator.h"
#include "Controllers/TerrainController.h"
#include "Drawables/Lights.h"

#include <iostream>
#include <vector>
#include <math.h>

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
Renderer renderer;

int main(void) {
	GLFWwindow* window;
	SceneContext sceneCtx;
	if (!sceneCtx.init(window)) {
		std::cout << "Failed to initialize SceneContext" << std::endl;
		return -1;
	}

	window = sceneCtx.getWindow();
	const char* title = std::string(glfwGetWindowTitle(window)).c_str();

	// window resize callback
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouseClick_callback);

	sceneCtx.setCamera(&camera);
	sceneCtx.setRenderer(&renderer);

	UiContext uiRenderer;
	uiRenderer.init(window);

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

		uiRenderer.preRender();
		sceneCtx.preRender();

		uiRenderer.render();	// Renders the Main Docking Window

		terrainController.DisplayUI();	// Renders the TerrainUI Windows
		terrainController.Update();

		sceneCtx.render();		// Renders the Scene Window

		uiRenderer.postRender();
		sceneCtx.postRender();
	}

	uiRenderer.shutdown();
	sceneCtx.shutdown();
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
