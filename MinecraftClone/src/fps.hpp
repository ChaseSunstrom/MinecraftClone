#ifndef FPS_HPP
#define FPS_HPP

#include "log.hpp"
#include "types.hpp"
#include <chrono>


namespace MC {
	struct FPSCounter {
		i32 frame_count = 0;
		f64 elapsed_time = 0.0;
		f64 fps = 0.0;
		std::chrono::time_point<std::chrono::steady_clock> last_time;

		FPSCounter() {
			last_time = std::chrono::steady_clock::now();
		}

		void Update() {
			frame_count++;
			auto current_time = std::chrono::steady_clock::now();
			std::chrono::duration<f64> delta = current_time - last_time;
			elapsed_time += delta.count();
			last_time = current_time;

			// Update FPS every second
			if (elapsed_time >= 1.0) {
				fps = frame_count / elapsed_time;
				LOG_INFO("FPS: " << fps);
				frame_count = 0;
				elapsed_time = 0.0;
			}
		}
	};
}


#endif