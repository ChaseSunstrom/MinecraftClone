#include "application.hpp"
#include "fps.hpp"


void EscapeFunction(MC::Application& app, MC::EventPtr<MC::KeyPressedEvent> event) {
	if (event->key == GLFW_KEY_ESCAPE) {
		app.Shutdown();
	}
}

void AddVoxels(MC::Application& app) {
	for (i32 i = 0; i < 50; i++) {
		for (i32 j = 0; j < 50; j++) {
			MC::Voxel voxel((MC::VoxelColor)(i % (i32)MC::VoxelColor::DARK_RED), MC::Transform(glm::vec3(i, 0.0f, j)));
			MC::Scene& scene = app.GetScene();
			scene.InsertVoxel(voxel);
		}
	}
}

static MC::VoxelColor color = MC::VoxelColor::GREEN;

void SwitchColor(MC::Application& app, MC::EventPtr<MC::KeyPressedEvent> event) {
	if (event->key >= GLFW_KEY_0 && event->key <= GLFW_KEY_9) {
		color = (MC::VoxelColor)(event->key % 9);
	}
}

void PlaceVoxel(MC::Application& app, MC::EventPtr<MC::MouseButtonPressedEvent> event) {
	if (event->button == GLFW_MOUSE_BUTTON_RIGHT) {
		MC::Scene& scene = app.GetScene();
		auto voxel_hit_opt = scene.GetVoxelLookedAt(100.0f); // Adjust max_distance as needed

		if (voxel_hit_opt.has_value()) {
			MC::VoxelHitInfo hit_info = voxel_hit_opt.value();
			MC::Voxel target_voxel = hit_info.voxel;
			MC::VoxelFace hit_face = hit_info.face;

			std::cout << "Target Voxel ID: " << target_voxel.GetID() << "\n";
			glm::vec3 target_pos = target_voxel.GetTransform().GetPos();
			std::cout << "Target Voxel Position: ("
				<< target_pos.x << ", "
				<< target_pos.y << ", "
				<< target_pos.z << ")\n";
			std::cout << "Hit Face: ";
			switch (hit_face) {
			case MC::VoxelFace::POS_X: std::cout << "POS_X"; break;
			case MC::VoxelFace::NEG_X: std::cout << "NEG_X"; break;
			case MC::VoxelFace::POS_Y: std::cout << "POS_Y"; break;
			case MC::VoxelFace::NEG_Y: std::cout << "NEG_Y"; break;
			case MC::VoxelFace::POS_Z: std::cout << "POS_Z"; break;
			case MC::VoxelFace::NEG_Z: std::cout << "NEG_Z"; break;
			}
			std::cout << "\n";

			// Determine the step direction based on the hit face
			glm::ivec3 step_dir(0, 0, 0);
			switch (hit_face) {
			case MC::VoxelFace::POS_X: step_dir.x = 1; break;
			case MC::VoxelFace::NEG_X: step_dir.x = -1; break;
			case MC::VoxelFace::POS_Y: step_dir.y = 1; break;
			case MC::VoxelFace::NEG_Y: step_dir.y = -1; break;
			case MC::VoxelFace::POS_Z: step_dir.z = 1; break;
			case MC::VoxelFace::NEG_Z: step_dir.z = -1; break;
			}

			// Calculate the new voxel position by adding the step direction
			glm::vec3 new_voxel_pos = target_pos + glm::vec3(step_dir);

			// Clamp the position to integer grid coordinates
			glm::ivec3 grid_pos = glm::ivec3(
				static_cast<i32>(std::round(new_voxel_pos.x)),
				static_cast<i32>(std::round(new_voxel_pos.y)),
				static_cast<i32>(std::round(new_voxel_pos.z))
			);

			std::cout << "New Voxel Position: ("
				<< new_voxel_pos.x << ", "
				<< new_voxel_pos.y << ", "
				<< new_voxel_pos.z << ")\n";
			std::cout << "Grid Position: ("
				<< grid_pos.x << ", "
				<< grid_pos.y << ", "
				<< grid_pos.z << ")\n";

			// Check if a voxel already exists at the new position
			auto existing_voxel = scene.GetVoxelAtPosition(grid_pos);
			if (!existing_voxel.has_value()) {
				// Insert the new voxel
				MC::Voxel new_voxel(color, MC::Transform(new_voxel_pos));
				scene.InsertVoxel(new_voxel);
				std::cout << "Placed Voxel at ("
					<< new_voxel_pos.x << ", "
					<< new_voxel_pos.y << ", "
					<< new_voxel_pos.z << ")\n";
			}
		}
		else {
			// Dont place voxels if none are looked at
		}
	}
}


void RemoveVoxel(MC::Application& app, MC::EventPtr<MC::MouseButtonPressedEvent> event) {
	if (event->button == GLFW_MOUSE_BUTTON_LEFT) {
		MC::Scene& scene = app.GetScene();
		auto voxel_hit_opt = scene.GetVoxelLookedAt(100.0f); // Adjust max_distance as needed

		if (voxel_hit_opt.has_value()) {
			MC::VoxelHitInfo hit_info = voxel_hit_opt.value();
			MC::Voxel target_voxel = hit_info.voxel;
			MC::VoxelFace hit_face = hit_info.face;

			std::cout << "Target Voxel ID: " << target_voxel.GetID() << "\n";
			glm::vec3 target_pos = target_voxel.GetTransform().GetPos();
			std::cout << "Target Voxel Position: ("
				<< target_pos.x << ", "
				<< target_pos.y << ", "
				<< target_pos.z << ")\n";
			std::cout << "Hit Face: ";
			switch (hit_face) {
			case MC::VoxelFace::POS_X: std::cout << "POS_X"; break;
			case MC::VoxelFace::NEG_X: std::cout << "NEG_X"; break;
			case MC::VoxelFace::POS_Y: std::cout << "POS_Y"; break;
			case MC::VoxelFace::NEG_Y: std::cout << "NEG_Y"; break;
			case MC::VoxelFace::POS_Z: std::cout << "POS_Z"; break;
			case MC::VoxelFace::NEG_Z: std::cout << "NEG_Z"; break;
			}
			std::cout << "\n";

			scene.RemoveVoxel(target_voxel.GetID());
			std::cout << "Removed Voxel with ID: " << target_voxel.GetID() << " at position ("
				<< target_pos.x << ", "
				<< target_pos.y << ", "
				<< target_pos.z << ")\n";
		}
		else {
			std::cout << "No voxel to remove in the line of sight.\n";
		}
	}
}

void MoveCameraOnKeyPress(MC::Application& app, MC::MultiEventPtr<MC::KeyPressedEvent, MC::KeyHeldEvent> event) {
	MC::Camera& camera = app.GetScene().GetCamera();

	f32 delta_time = 0.5f;

	if (event->Is<MC::KeyPressedEvent>()) {
		auto actual_event = event->Get<MC::KeyPressedEvent>();
		switch (actual_event->key) {
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

	else if (event->Is<MC::KeyHeldEvent>()) {
		auto actual_event = event->Get<MC::KeyHeldEvent>();
		switch (actual_event->key) {
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
}

static bool first_mouse = true;
static f32 lastx = 500.0f; // Initial horizontal position
static f32 lasty = 500.0f; // Initial vertical position

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

	f32 mouse_sensitivity = camera.GetMouseSensitivity();
	xoffset *= mouse_sensitivity;
	yoffset *= mouse_sensitivity;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

i32 main() {
	MC::Application app;

	MC::FPSCounter fps_counter;

	// Add features via method chaining
	app.CreateWindow("Minecraft Clone", 1000, 1000)
		.AddStartupFunction([](MC::Application& app) {
				LOG_INFO("Application initialized!");
			})
		.AddStartupFunction(AddVoxels)
		.AddShutdownFunction([](MC::Application& app) {
				LOG_INFO("Application shut down!");
			})
		.AddUpdateFunction([&fps_counter](MC::Application& app) {
				fps_counter.Update();
			})
		.AddEventFunction<MC::KeyPressedEvent>(SwitchColor)
		.AddEventFunction<MC::KeyPressedEvent>(EscapeFunction)
		.AddEventFunction<MC::MouseButtonPressedEvent>([&](MC::Application& app, MC::EventPtr<MC::MouseButtonPressedEvent> event) {
					if (event->button == GLFW_MOUSE_BUTTON_RIGHT) {
						PlaceVoxel(app, event);
					}
					else if (event->button == GLFW_MOUSE_BUTTON_LEFT) {
						RemoveVoxel(app, event);
					}
			})
		.AddEventFunction<MC::KeyPressedEvent, MC::KeyHeldEvent>(MoveCameraOnKeyPress)
		.AddEventFunction<MC::MouseMovedEvent>(RotateCameraOnMouseMove)
		.Start();
}