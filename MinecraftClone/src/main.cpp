#include "application.hpp"
#include "fps.hpp"

#include <GLM/gtc/noise.hpp>


void EscapeFunction(MC::Application& app, MC::EventPtr<MC::KeyPressedEvent> event) {
	if (event->key == GLFW_KEY_ESCAPE) {
		app.Shutdown();
	}
}

void GenerateWorld(MC::Application& app) {
	MC::Scene& scene = app.GetScene();

	// Parameters for world generation
	const i32 world_width = 100;
	const i32 world_depth = 100;
	const f32 scale = 0.1f; // Scale of the noise
	const i32 max_height = 20; // Maximum height of terrain

	// Parameters for biome generation
	const f32 biome_scale = 0.01f; // Larger scale for biomes

	for (i32 x = 0; x < world_width; ++x) {
		for (i32 z = 0; z < world_depth; ++z) {
			// Generate height using Perlin noise
			f32 noise_value = glm::perlin(glm::vec2(x * scale, z * scale));
			// Map noiseValue from [-1,1] to [0,1]
			noise_value = (noise_value + 1.0f) / 2.0f;
			// Compute height
			i32 height = static_cast<i32>(noise_value * max_height);

			// Generate biome using another layer of Perlin noise
			f32 biome_noise = glm::perlin(glm::vec2(x * biome_scale, z * biome_scale));
			// Map biomeNoise from [-1,1] to [0,1]
			biome_noise = (biome_noise + 1.0f) / 2.0f;

			// Determine biome based on biomeNoise
			enum BiomeType {
				DESERT,
				PLAINS,
				FOREST,
				SNOWY,
				MOUNTAINS,
				OCEAN
			};
			BiomeType biome;
			if (biome_noise < 0.2f) {
				biome = DESERT;
			}
			else if (biome_noise < 0.4f) {
				biome = PLAINS;
			}
			else if (biome_noise < 0.6f) {
				biome = FOREST;
			}
			else if (biome_noise < 0.8f) {
				biome = SNOWY;
			}
			else {
				biome = MOUNTAINS;
			}

			// Adjust height based on biome
			if (biome == OCEAN) {
				height = 3; // Low height for ocean
			}
			else if (biome == MOUNTAINS) {
				height += 10; // Increase height for mountains
			}
			else if (biome == DESERT) {
				height -= 2; // Slightly lower for desert
			}

			// Clamp height
			if (height < 1) height = 1;
			if (height > max_height) height = max_height;

			// For each y from 0 up to height, create a voxel
			for (i32 y = 0; y <= height; ++y) {
				// Determine voxel color based on height and biome
				MC::VoxelColor voxel_color;

				if (biome == OCEAN) {
					voxel_color = MC::VoxelColor::BLUE; // Water
				}
				else if (biome == DESERT) {
					if (y == height) {
						voxel_color = MC::VoxelColor::YELLOW; // Sand
					}
					else {
						voxel_color = MC::VoxelColor::BROWN; // Dirt
					}
				}
				else if (biome == PLAINS) {
					if (y == height) {
						voxel_color = MC::VoxelColor::GREEN; // Grass
					}
					else {
						voxel_color = MC::VoxelColor::BROWN; // Dirt
					}
				}
				else if (biome == FOREST) {
					if (y == height) {
						voxel_color = MC::VoxelColor::GREEN; // Grass
					}
					else {
						voxel_color = MC::VoxelColor::BROWN; // Dirt
					}
				}
				else if (biome == SNOWY) {
					if (y == height) {
						voxel_color = MC::VoxelColor::WHITE; // Snow
					}
					else {
						voxel_color = MC::VoxelColor::GRAY; // Stone
					}
				}
				else if (biome == MOUNTAINS) {
					if (y >= height - 2) {
						voxel_color = MC::VoxelColor::WHITE; // Snow
					}
					else if (y >= height - 5) {
						voxel_color = MC::VoxelColor::GRAY; // Stone
					}
					else {
						voxel_color = MC::VoxelColor::BROWN; // Dirt
					}
				}

				// Create the voxel
				MC::Voxel voxel(voxel_color, MC::Transform(glm::vec3(x, y, z)));
				// Insert the voxel into the scene
				scene.InsertVoxel(voxel);
			}
		}
	}
}


void ZoomCamera(MC::Application& app, MC::EventPtr<MC::MouseScrolledEvent> event) {
	MC::Camera& camera = app.GetScene().GetCamera();
	camera.SetFOV(camera.GetFOV() - event->y);
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
		.AddStartupFunction(GenerateWorld)
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
		.AddEventFunction<MC::MouseScrolledEvent>(ZoomCamera)
		.Start();
}