#include "application.hpp"

i32 main() {

	// App uses builder pattern for adding stuff
	MC::Application app;

	app.CreateWindow("Minecraft Clone", 1000, 1000)
		.AddShutdownFunction([](MC::Application& app) {
		std::cout << "Application initialized!";
		})
		.Start();


}