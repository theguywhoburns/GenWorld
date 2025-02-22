#include <ImGuiCurveTest.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <Shader.h>
#include <Camera.h>
#include <VAO.h>
#include <Texture.h>
#include <BufferObject.h>
#include <Model.h>
#include <TerrainGenerator.h>
#include <Lights.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <glm/gtc/noise.hpp>
#include <random>

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

// GUI Functions
bool SettingsGUI();
bool ColorSettingsGUI();

// Terrain Data
float width = 100;
float length = 100;
int divisionSize = 1;
float terrainHeightMultiplier = 7;
ImGui::point v[4] = { {0.0f, 0.0f}, {0.5f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.5f} };

// Noise Data
float lacunarity = 2.0f; // Adjust for frequency of noise
float persistence = 0.5f; // Adjust for amplitude of noise
float scale = 50.0f; // Adjust for overall height of terrain
int octaves = 4; // Adjust for number of noise layers
int seed = 0;
glm::vec2 offset = glm::vec2(0.0f, 0.0f);

std::vector<TerrainUtilities::VertexColor> colors = {
	{0.1f, glm::vec4(0.15f, 0.22f, 0.34f, 1.0f)},  // Deep Water (Dark Blue)
	{0.15f, glm::vec4(0.2f, 0.4f, 0.6f, 1.0f)},   // Shallow Water (Lighter Blue)
	{0.25f, glm::vec4(0.8f, 0.7f, 0.4f, 1.0f)},   // Sand (Yellowish)
	{0.35f, glm::vec4(0.3f, 0.6f, 0.2f, 1.0f)},   // Grass (Green)
	{0.5f, glm::vec4(0.4f, 0.7f, 0.3f, 1.0f)},    // Lush Grass (Brighter Green)
	{0.65f, glm::vec4(0.5f, 0.4f, 0.2f, 1.0f)},   // Dirt (Brown)
	{0.9f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)},    // Rock (Gray)
	{1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)},   // Snow (White)
};

Mesh* terrain;
TerrainGenerator terrainGenerator;
void InitializeTerrain() {
	terrainGenerator.SetTerrainData(width, length, divisionSize, terrainHeightMultiplier, v);
	terrainGenerator.SetNoiseParameters(lacunarity, persistence, scale);
	terrainGenerator.SetNoiseSettings(octaves, seed, offset);
	terrainGenerator.SetColorData(colors);

	terrain = terrainGenerator.GenerateTerrain();
}

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

	Shader ourShader("Shaders/Terrain.vert", "Shaders/Terrain.frag");

	// Plane Data
	InitializeTerrain();

	unsigned int planeVAO, planeVBO, planeEBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glGenBuffers(1, &planeEBO);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window)) {
		calculateDeltaTime();
		processInput(window);
		calculateMousePos(window);
		updateTitle(window, title, calculateFPS(window));

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		/* Render here */
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // background color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// ImGui Windows
		bool settingsChanged = false;
		settingsChanged |= SettingsGUI() | ColorSettingsGUI();
		if (settingsChanged) {
			delete terrain;

			terrain = terrainGenerator.GenerateTerrain();
		}

		glm::mat4 model = glm::mat4(1.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);

		ourShader.use();
		ourShader.setMat4("model", model);
		ourShader.setMat4("view", view);
		ourShader.setMat4("projection", projection);

		terrain->Draw(ourShader);

		// ImGui Rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &planeEBO);
	glDeleteVertexArrays(1, &planeVAO);

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

bool SettingsGUI() {
	bool valuesChanged = false;
	bool noiseParamsChanged = false;
	bool noiseSettingsChanged = false;

	ImGui::Begin("Terrain Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
	ImGui::Text("Terrain Settings");
	if (ImGui::DragFloat("Width", &width, 0.1f, 1, 100) ||
		ImGui::DragFloat("Length", &length, 0.1f, 1, 100) ||
		ImGui::SliderInt("Division Size", &divisionSize, 1, 10) ||
		ImGui::DragFloat("Height Multiplier", &terrainHeightMultiplier, 0.1f, 1, 100) ||
		ImGui::DrawCurve("easeOutSine", v)       // draw
		) {
		valuesChanged = true;
		terrainGenerator.SetTerrainData(width, length, divisionSize, terrainHeightMultiplier, v);
	}

	ImGui::Separator();
	ImGui::NewLine();

	ImGui::Text("Noise Settings");
	noiseParamsChanged |= ImGui::SliderFloat("Scale", &scale, 0.001f, 50.0f);
	noiseSettingsChanged |= ImGui::SliderInt("Octaves", &octaves, 1, 10);
	noiseParamsChanged |= ImGui::SliderFloat("Lacunarity", &lacunarity, 0.1f, 50.0f);
	noiseParamsChanged |= ImGui::SliderFloat("Persistence", &persistence, 0.0f, 1.0f);
	noiseSettingsChanged |= ImGui::DragFloat2("Offset", &offset[0], 0.1f);
	noiseSettingsChanged |= ImGui::DragInt("Seed", &seed, 1, 0, 10000);

	if (noiseParamsChanged) {
		terrainGenerator.SetNoiseParameters(lacunarity, persistence, scale);
	}

	ImGui::Separator();
	ImGui::NewLine();

	// Make the button bigger
	if (ImGui::Button("Randomize Seed", ImVec2(200, 40))) {
		seed = rand() % 10000;
		noiseSettingsChanged = true;
	}
	ImGui::End();

	if (noiseSettingsChanged) {
		terrainGenerator.SetNoiseSettings(octaves, seed, offset);
	}

	return valuesChanged || noiseParamsChanged || noiseSettingsChanged;
}

bool ColorSettingsGUI() {
	bool valuesChanged = false;

	ImGui::Begin("Color Settings", nullptr, ImGuiWindowFlags_NoFocusOnAppearing);
	ImGui::Text("Color Settings");

	for (size_t i = 0; i < colors.size(); i++) {
		std::string index = std::to_string(i + 1);
		ImGui::Text(("Color " + index).c_str());

		// Color picker & height slider
		valuesChanged |=
			ImGui::ColorEdit3(("Color##" + index).c_str(), &colors[i].color[0]) |
			ImGui::SliderFloat(("Height##" + index).c_str(), &colors[i].height, 0.0f, 1.0f);

		ImGui::SameLine();
		if (ImGui::Button(("X##" + index).c_str())) {
			colors.erase(colors.begin() + i);
			valuesChanged = true;
			continue;
		}

		// Reordering buttons
		if (i > 0) {
			ImGui::SameLine();
			if (ImGui::Button(("▲##" + index).c_str())) {
				std::swap(colors[i], colors[i - 1]);
				valuesChanged = true;
			}
		}
		if (i < colors.size() - 1) {
			ImGui::SameLine();
			if (ImGui::Button(("▼##" + index).c_str())) {
				std::swap(colors[i], colors[i + 1]);
				valuesChanged = true;
			}
		}
	}

	// Add new color button
	if (ImGui::Button("Add Color")) {
		TerrainUtilities::VertexColor defaultColor;
		defaultColor.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		defaultColor.height = 0.5f;
		colors.push_back(defaultColor);
		valuesChanged = true;
	}

	ImGui::End();

	if (valuesChanged) {
		terrainGenerator.SetColorData(colors);
	}

	return valuesChanged;
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
