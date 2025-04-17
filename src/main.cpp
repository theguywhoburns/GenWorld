#include "Core/Engine/Application.h"

int main(void) {
	Application* app = Application::GetInstance();
	app->Run();

	delete app;

	return 0;
}
