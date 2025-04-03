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
#include "Utils/Time.h"

#include <iostream>
#include <vector>
#include <math.h>

Camera camera(glm::vec3(0.0f, 20.0f, 20.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -45.0f);
Renderer renderer;

int main(void) {
	SceneContext sceneCtx;
	if (!sceneCtx.init(nullptr)) {
		std::cout << "Failed to initialize SceneContext" << std::endl;
		return -1;
	}

	GLFWwindow* window = sceneCtx.getWindow();

	sceneCtx.setCamera(&camera);
	sceneCtx.setRenderer(&renderer);

	UiContext uiRenderer;
	uiRenderer.init(window);

	Shader ourShader("Shaders/Terrain.vert", "Shaders/Terrain.frag");
	renderer.SetShader(&ourShader);
	renderer.SetCamera(&camera);

	// Plane Data
	TerrainController terrainController(&renderer);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window)) {
		Utils::Time::Update();

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
