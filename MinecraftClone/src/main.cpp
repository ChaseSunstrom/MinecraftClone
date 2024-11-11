#include "application.hpp"

void EscapeFunction(MC::Application& app, MC::EventPtr<MC::KeyPressedEvent> event) {
	if (event->key == GLFW_KEY_ESCAPE) {
		app.Shutdown();
	}
}

i32 main() {

	// App uses builder pattern for adding stuff
	MC::Application app;

	app.CreateWindow("Minecraft Clone", 1000, 1000)
		.AddStartupFunction([](MC::Application& app) {
			std::cout << "Application initialized!\n";
		})
		.AddShutdownFunction([](MC::Application& app) {
			std::cout << "Application shut down!\n";
		})
		.AddEventFunction<MC::KeyPressedEvent>(EscapeFunction)
		.Start();


}