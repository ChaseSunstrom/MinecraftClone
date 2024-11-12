#include "scene.hpp"
#include "ray.hpp"
#include <algorithm>
#include <iostream>

namespace MC {
    Scene::Scene(EventHandler& event_handler)
        : m_event_handler(event_handler),
        m_camera(std::make_unique<Camera>()),
        m_sky_color(glm::vec4(0.2f, 0.3f, 0.4f, 1.0f)) {
        Camera& camera = *m_camera;
        m_event_handler.SubscribeToEvent<WindowResizedEvent>([&camera](EventPtr<WindowResizedEvent> event) {
            camera.OnWindowResize(event);
            });
    }

    Scene::~Scene() {
        Voxel::CleanupStaticBuffers();
    }

    void Scene::InitializeScene() {
        Voxel::InitializeStaticBuffers();
    }

    void Scene::SetSkyColor(const glm::vec4& sky_color) {
        m_sky_color = sky_color;
    }

    glm::vec4 Scene::GetSkyColor() const {
        return m_sky_color;
    }

    void Scene::InsertVoxel(const Voxel& voxel) {
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        u32 id = voxel.GetID();
        glm::ivec3 grid_pos = glm::ivec3(
            static_cast<i32>(std::floor(voxel.GetTransform().GetPos().x)),
            static_cast<i32>(std::floor(voxel.GetTransform().GetPos().y)),
            static_cast<i32>(std::floor(voxel.GetTransform().GetPos().z))
        );

        // Check if a voxel already exists at the grid position
        if (m_position_to_id.find(grid_pos) != m_position_to_id.end()) {
            std::cerr << "Cannot insert voxel: A voxel already exists at ("
                << grid_pos.x << ", "
                << grid_pos.y << ", "
                << grid_pos.z << ").\n";
            return;
        }

        m_voxels.emplace(voxel.GetID(), voxel);
        m_voxel_transforms[voxel.GetVoxelColor()].push_back(voxel.GetTransform());
        m_voxel_matrices[voxel.GetVoxelColor()].push_back(voxel.GetTransform().GetTransform());

        // Map grid position to voxel ID
        m_position_to_id.emplace(grid_pos, voxel.GetID());
    }
    void Scene::RemoveVoxel(u32 voxel_id) {
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        auto it = m_voxels.find(voxel_id);
        if (it == m_voxels.end()) {
            std::cerr << "Attempted to remove non-existent voxel with ID: " << voxel_id << "\n";
            return;
        }

        Voxel voxel_to_remove = it->second;

        // Remove from m_voxel_transforms
        auto& transforms = m_voxel_transforms[voxel_to_remove.GetVoxelColor()];
        transforms.erase(std::remove(transforms.begin(), transforms.end(), voxel_to_remove.GetTransform()), transforms.end());

        // Remove from m_voxel_matrices
        auto& matrices = m_voxel_matrices[voxel_to_remove.GetVoxelColor()];
        matrices.erase(std::remove(matrices.begin(), matrices.end(), voxel_to_remove.GetTransform().GetTransform()), matrices.end());

        // Remove from m_position_to_id
        glm::vec3 pos = voxel_to_remove.GetTransform().GetPos();
        glm::ivec3 grid_pos = glm::ivec3(
            static_cast<i32>(std::floor(pos.x)),
            static_cast<i32>(std::floor(pos.y)),
            static_cast<i32>(std::floor(pos.z))
        );
        m_position_to_id.erase(grid_pos);

        // Remove from m_voxels
        m_voxels.erase(it);
    }

    void Scene::UpdateVoxel(const Voxel& updated_voxel) {
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        u32 voxel_id = updated_voxel.GetID();
        auto it = m_voxels.find(voxel_id);
        if (it == m_voxels.end()) {
            std::cerr << "Attempted to update non-existent voxel with ID: " << voxel_id << "\n";
            return;
        }

        Voxel& voxel = it->second;

        // Remove old transform and matrix
        auto& old_transforms = m_voxel_transforms[voxel.GetVoxelColor()];
        old_transforms.erase(std::remove(old_transforms.begin(), old_transforms.end(), voxel.GetTransform()), old_transforms.end());

        auto& old_matrices = m_voxel_matrices[voxel.GetVoxelColor()];
        old_matrices.erase(std::remove(old_matrices.begin(), old_matrices.end(), voxel.GetTransform().GetTransform()), old_matrices.end());

        // Remove old position mapping
        glm::vec3 old_pos = voxel.GetTransform().GetPos();
        glm::ivec3 old_grid_pos = glm::ivec3(
            static_cast<i32>(std::floor(old_pos.x)),
            static_cast<i32>(std::floor(old_pos.y)),
            static_cast<i32>(std::floor(old_pos.z))
        );
        m_position_to_id.erase(old_grid_pos);

        // Update voxel properties
        voxel = updated_voxel;

        // Re-insert updated transform and matrix
        m_voxel_transforms[voxel.GetVoxelColor()].push_back(voxel.GetTransform());
        m_voxel_matrices[voxel.GetVoxelColor()].push_back(voxel.GetTransform().GetTransform());

        // Add new position mapping
        glm::vec3 new_pos = voxel.GetTransform().GetPos();
        glm::ivec3 new_grid_pos = glm::ivec3(
            static_cast<i32>(std::floor(new_pos.x)),
            static_cast<i32>(std::floor(new_pos.y)),
            static_cast<i32>(std::floor(new_pos.z))
        );
        m_position_to_id.emplace(new_grid_pos, voxel_id);
    }

    std::optional<Voxel> Scene::GetVoxel(u32 id) const {
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        auto it = m_voxels.find(id);
        if (it == m_voxels.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    Camera& Scene::GetCamera() const {
        return *m_camera;
    }

    const std::unordered_map<VoxelColor, std::vector<Transform>>& Scene::GetVoxelTransforms() const {
        return m_voxel_transforms;
    }

    const std::unordered_map<VoxelColor, std::vector<glm::mat4>>& Scene::GetVoxelMatrices() const {
        return m_voxel_matrices;
    }

    const std::unordered_map<u32, Voxel>& Scene::GetVoxels() const {
        return m_voxels;
    }

    std::optional<Voxel> Scene::GetVoxelAtPosition(const glm::ivec3& grid_pos) const {
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        auto it = m_position_to_id.find(grid_pos);
        if (it != m_position_to_id.end()) {
            u32 voxel_id = it->second;
            auto voxel_it = m_voxels.find(voxel_id);
            if (voxel_it != m_voxels.end()) {
                return voxel_it->second;
            }
        }
        return std::nullopt;
    }

    std::optional<VoxelHitInfo> Scene::GetVoxelLookedAt(f32 max_distance) const {
        Camera& camera = GetCamera();
        glm::vec3 origin = camera.GetPosition();
        glm::vec3 direction = glm::normalize(camera.GetFront());

        Ray ray(origin, direction);
        glm::ivec3 current_voxel = glm::ivec3(
            static_cast<i32>(std::floor(ray.origin.x)),
            static_cast<i32>(std::floor(ray.origin.y)),
            static_cast<i32>(std::floor(ray.origin.z))
        );

        glm::vec3 t_max;
        glm::vec3 t_delta;
        glm::ivec3 step;
        i32 last_step_axis = -1; // Initialize with an invalid axis

        for (i32 i = 0; i < 3; ++i) {
            if (ray.direction[i] > 0.0f) {
                step[i] = 1;
                t_max[i] = (static_cast<f32>(current_voxel[i] + 1) - ray.origin[i]) / ray.direction[i];
                t_delta[i] = 1.0f / ray.direction[i];
            }
            else if (ray.direction[i] < 0.0f) {
                step[i] = -1;
                t_max[i] = (ray.origin[i] - static_cast<f32>(current_voxel[i])) / -ray.direction[i];
                t_delta[i] = 1.0f / -ray.direction[i];
            }
            else {
                step[i] = 0;
                t_max[i] = std::numeric_limits<f32>::infinity();
                t_delta[i] = std::numeric_limits<f32>::infinity();
            }
        }

        f32 distance_traveled = 0.0f;
        i32 step_count = 0;

        while (distance_traveled < max_distance) {
            glm::ivec3 grid_pos = current_voxel;
            auto voxel_opt = GetVoxelAtPosition(grid_pos);
            if (voxel_opt.has_value()) {
                // Determine hit_face based on last stepped axis
                VoxelFace hit_face;
                switch (last_step_axis) {
                case 0:
                    hit_face = (step.x > 0) ? VoxelFace::NEG_X : VoxelFace::POS_X;
                    break;
                case 1:
                    hit_face = (step.y > 0) ? VoxelFace::NEG_Y : VoxelFace::POS_Y;
                    break;
                case 2:
                    hit_face = (step.z > 0) ? VoxelFace::NEG_Z : VoxelFace::POS_Z;
                    break;
                default:
                    hit_face = VoxelFace::POS_X; // Default value
                }

                VoxelHitInfo hit_info{ voxel_opt.value(), hit_face };
                return hit_info;
            }

            // Determine which axis to step
            i32 axis = 0;
            if (t_max.y < t_max.x) {
                if (t_max.z < t_max.y) {
                    axis = 2;
                }
                else {
                    axis = 1;
                }
            }
            else {
                if (t_max.z < t_max.x) {
                    axis = 2;
                }
                else {
                    axis = 0;
                }
            }

            // Step to the next voxel
            current_voxel[axis] += step[axis];
            distance_traveled = t_max[axis];
            t_max[axis] += t_delta[axis];
            last_step_axis = axis;

            step_count++;
        }

        return std::nullopt;
    }


}
