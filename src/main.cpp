#define IMGUI_DEFINE_MATH_OPERATORS
#define GLFW_INCLUDE_NONE

#include "Renderers/UiContext.h"
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


int main(void) {
	// Initialize the application
	if (!Application::getInstance()->init()) {
		std::cout << "Failed to initialize Application" << std::endl;
		return -1;
	}
	
	Application::getInstance()->run();

	Application::getInstance()->terminate();


	return 0;
}
