#include "application.hpp"

void EscapeFunction(MC::Application& app, MC::EventPtr<MC::KeyPressedEvent> event) {
	if (event->key == GLFW_KEY_ESCAPE) {
		app.Shutdown();
	}
}

void AddVoxels(MC::Application& app) {
	MC::Voxel voxel(MC::VoxelColor::GREEN, MC::Transform(glm::vec3(0.0f, 0.0f, 0.0f)));
	MC::Scene& scene = app.GetScene();
	scene.InsertVoxel(voxel);
	std::cout << "Inserted Voxel with ID: " << voxel.GetID() << "\n";
}


void RotateVoxel(MC::Application& app, u32 voxel_id, const glm::vec3& rotation_increment) {
    MC::Scene& scene = app.GetScene();

    // Retrieve the voxel by its unique ID
    std::optional<MC::Voxel> voxel_opt = scene.GetVoxel(voxel_id);
    if (!voxel_opt.has_value()) {
        std::cerr << "Error: Voxel with ID " << voxel_id << " not found.\n";
        return;
    }

    // Create a copy of the voxel to modify
    MC::Voxel voxel = voxel_opt.value();

    // Apply the rotation increment
    voxel.Rotate(rotation_increment);

    // Update the voxel in the scene
    scene.UpdateVoxel(voxel);

    // Optional: Log the new rotation for verification
    std::optional<MC::Voxel> updated_voxel_opt = scene.GetVoxel(voxel_id);
    if (updated_voxel_opt.has_value()) {
        MC::Voxel updated_voxel = updated_voxel_opt.value();
        glm::vec3 new_rot = updated_voxel.GetRot();
        std::cout << "Voxel with ID " << voxel_id << " rotated to ("
            << new_rot.x << ", "
            << new_rot.y << ", "
            << new_rot.z << ") degrees.\n";
    }
    else {
        std::cerr << "Error: Failed to retrieve updated voxel with ID " << voxel_id << ".\n";
    }
}

void MoveCameraOnKeyPress(MC::Application& app, MC::EventPtr<MC::KeyPressedEvent> event) {
    MC::Camera& camera = app.GetScene().GetCamera();

    f32 delta_time = 0.3f;

    // Determine the direction based on the key pressed
    switch (event->key) {
    case GLFW_KEY_W:
        camera.ProcessKeyboard(MC::CameraMovement::FORWARD, delta_time);
        break;
    case GLFW_KEY_S:
        camera.ProcessKeyboard(MC::CameraMovement::BACKWARD, delta_time);
        break;
    case GLFW_KEY_A:
        camera.ProcessKeyboard(MC::CameraMovement::LEFT, delta_time);
        break;
    case GLFW_KEY_D:
        camera.ProcessKeyboard(MC::CameraMovement::RIGHT, delta_time);
        break;
    case GLFW_KEY_SPACE:
        camera.ProcessKeyboard(MC::CameraMovement::UP, delta_time);
        break;
    case GLFW_KEY_LEFT_SHIFT:
        camera.ProcessKeyboard(MC::CameraMovement::DOWN, delta_time);
        break;

    }
}


static bool first_mouse = true;
static f32 lastx = 400.0f; // Initial horizontal position
static f32 lasty = 300.0f; // Initial vertical position

void RotateCameraOnMouseMove(MC::Application& app, MC::EventPtr<MC::MouseMovedEvent> event) {
    MC::Camera& camera = app.GetScene().GetCamera();

    f32 xpos = static_cast<f32>(event->xpos);
    f32 ypos = static_cast<f32>(event->ypos);

    if (first_mouse) {
        lastx = xpos;
        lasty = ypos;
        first_mouse = false;
    }

    f32 xoffset = xpos - lastx;
    f32 yoffset = lasty - ypos; // Reversed since y-coordinates go from bottom to top
    lastx = xpos;
    lasty = ypos;

    f32 mouse_sensitivity = camera.GetMouseSensitivity(); // Assume getter exists
    xoffset *= mouse_sensitivity;
    yoffset *= mouse_sensitivity;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

i32 main() {
	MC::Application app;

	app.CreateWindow("Minecraft Clone", 1920, 1080)
		.AddStartupFunction([](MC::Application& app) {
			std::cout << "Application initialized!\n";
		})
		.AddStartupFunction(AddVoxels)
		.AddShutdownFunction([](MC::Application& app) {
			std::cout << "Application shut down!\n";
		})
        .AddUpdateFunction([](MC::Application& app) {
             glm::vec3 rotation_increment(0.0f, 0.005f, 0.0f);
             RotateVoxel(app, 1, rotation_increment);
        })
		.AddEventFunction<MC::KeyPressedEvent>(EscapeFunction)
        .AddEventFunction<MC::KeyPressedEvent>(MoveCameraOnKeyPress)
        .AddEventFunction<MC::MouseMovedEvent>(RotateCameraOnMouseMove)
		.Start();


}