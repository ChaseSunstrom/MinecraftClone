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
        glm::ivec3 grid_pos = glm::ivec3(glm::floor(voxel.GetTransform().GetPos()));

        // Check for existing voxel at the position
        if (m_position_to_id.find(grid_pos) != m_position_to_id.end()) {
            std::cerr << "Cannot insert voxel: A voxel already exists at ("
                << grid_pos.x << ", " << grid_pos.y << ", " << grid_pos.z << ").\n";
            return;
        }

        // Copy the voxel and initialize all faces as visible
        Voxel voxel_copy = voxel;
        voxel_copy.visible_faces = 0x3F; // All faces visible

        // Directions for neighboring voxels
        static const glm::ivec3 directions[6] = {
            {1, 0, 0},  // POS_X
            {-1, 0, 0}, // NEG_X
            {0, 1, 0},  // POS_Y
            {0, -1, 0}, // NEG_Y
            {0, 0, 1},  // POS_Z
            {0, 0, -1}  // NEG_Z
        };

        // Update visibility based on neighbors
        for (int i = 0; i < 6; ++i) {
            glm::ivec3 neighbor_pos = grid_pos + directions[i];
            auto neighbor_it = m_position_to_id.find(neighbor_pos);
            if (neighbor_it != m_position_to_id.end()) {
                u32 neighbor_id = neighbor_it->second;
                auto voxel_it = m_voxels.find(neighbor_id);
                if (voxel_it != m_voxels.end()) {
                    Voxel& neighbor_voxel = voxel_it->second;
                    // Hide the adjacent faces
                    neighbor_voxel.SetFaceVisible(static_cast<Voxel::FaceIndex>((i ^ 1)), false);
                    voxel_copy.SetFaceVisible(static_cast<Voxel::FaceIndex>(i), false);
                }
            }
        }

        // Insert the voxel
        m_voxels.emplace(voxel_copy.GetID(), voxel_copy);
        m_voxel_transforms[voxel_copy.GetVoxelColor()].push_back(voxel_copy.GetTransform());
        m_voxel_matrices[voxel_copy.GetVoxelColor()].push_back(voxel_copy.GetTransform().GetTransform());
        m_position_to_id.emplace(grid_pos, voxel_copy.GetID());
    }

    void Scene::InsertVoxels(const std::vector<Voxel>& voxels) {
        // Temporary data structures
        std::unordered_map<glm::ivec3, u32> temp_position_to_id;
        std::unordered_map<u32, Voxel> temp_voxels;

        // Insert voxels into temporary data structures
        for (const Voxel& voxel : voxels) {
            glm::ivec3 grid_pos = glm::ivec3(glm::floor(voxel.GetTransform().GetPos()));

            // Skip if voxel already exists in temp data
            if (temp_position_to_id.find(grid_pos) != temp_position_to_id.end()) {
                continue;
            }

            // Initialize all faces as visible
            Voxel voxel_copy = voxel;
            voxel_copy.visible_faces = 0x3F; // All faces visible

            temp_voxels.emplace(voxel_copy.GetID(), voxel_copy);
            temp_position_to_id.emplace(grid_pos, voxel_copy.GetID());
        }

        // Update face visibility based on neighbors in temp and scene data
        for (auto& temp_voxel_pair : temp_voxels) {
            u32 voxel_id = temp_voxel_pair.first;
            Voxel& voxel = temp_voxel_pair.second;

            glm::ivec3 grid_pos = glm::ivec3(glm::floor(voxel.GetTransform().GetPos()));
            static const glm::ivec3 directions[6] = {
                {1, 0, 0},  // POS_X
                {-1, 0, 0}, // NEG_X
                {0, 1, 0},  // POS_Y
                {0, -1, 0}, // NEG_Y
                {0, 0, 1},  // POS_Z
                {0, 0, -1}  // NEG_Z
            };

            for (int i = 0; i < 6; ++i) {
                glm::ivec3 neighbor_pos = grid_pos + directions[i];

                // Check in temp data
                auto temp_neighbor_it = temp_position_to_id.find(neighbor_pos);
                if (temp_neighbor_it != temp_position_to_id.end()) {
                    u32 neighbor_id = temp_neighbor_it->second;

                    auto neighbor_voxel_it = temp_voxels.find(neighbor_id);
                    if (neighbor_voxel_it != temp_voxels.end()) {
                        Voxel& neighbor_voxel = neighbor_voxel_it->second;

                        // Hide adjacent faces
                        neighbor_voxel.SetFaceVisible(static_cast<Voxel::FaceIndex>((i ^ 1)), false);
                        voxel.SetFaceVisible(static_cast<Voxel::FaceIndex>(i), false);
                    }
                }
                else {
                    // Check in existing scene data
                    std::lock_guard<std::mutex> lock(m_voxel_mutex);
                    auto scene_neighbor_it = m_position_to_id.find(neighbor_pos);
                    if (scene_neighbor_it != m_position_to_id.end()) {
                        u32 neighbor_id = scene_neighbor_it->second;

                        auto neighbor_voxel_it = m_voxels.find(neighbor_id);
                        if (neighbor_voxel_it != m_voxels.end()) {
                            Voxel& neighbor_voxel = neighbor_voxel_it->second;

                            // Hide adjacent faces
                            neighbor_voxel.SetFaceVisible(static_cast<Voxel::FaceIndex>((i ^ 1)), false);
                            voxel.SetFaceVisible(static_cast<Voxel::FaceIndex>(i), false);
                        }
                    }
                }
            }
        }

        // Lock and insert temp data into scene data structures
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        for (auto& temp_voxel_pair : temp_voxels) {
            u32 voxel_id = temp_voxel_pair.first;
            Voxel& voxel = temp_voxel_pair.second;

            m_voxels.emplace(voxel_id, voxel);
            glm::ivec3 grid_pos = glm::ivec3(glm::floor(voxel.GetTransform().GetPos()));
            m_position_to_id.emplace(grid_pos, voxel_id);
            m_voxel_transforms[voxel.GetVoxelColor()].push_back(voxel.GetTransform());
            m_voxel_matrices[voxel.GetVoxelColor()].push_back(voxel.GetTransform().GetTransform());
        }
    }

    void Scene::RemoveVoxel(u32 voxel_id) {
        std::lock_guard<std::mutex> lock(m_voxel_mutex);
        auto it = m_voxels.find(voxel_id);
        if (it == m_voxels.end()) {
            LOG_ERROR("Attempted to remove non-existent voxel with ID: " << voxel_id);
            return;
        }

        Voxel voxel_to_remove = it->second;
        glm::ivec3 grid_pos = glm::ivec3(glm::floor(voxel_to_remove.GetTransform().GetPos()));

        // Directions for neighboring voxels
        static const glm::ivec3 directions[6] = {
            {1, 0, 0},  // POS_X
            {-1, 0, 0}, // NEG_X
            {0, 1, 0},  // POS_Y
            {0, -1, 0}, // NEG_Y
            {0, 0, 1},  // POS_Z
            {0, 0, -1}  // NEG_Z
        };

        // Update neighbors to make their faces visible
        for (int i = 0; i < 6; ++i) {
            glm::ivec3 neighbor_pos = grid_pos + directions[i];
            auto neighbor_it = m_position_to_id.find(neighbor_pos);
            if (neighbor_it != m_position_to_id.end()) {
                u32 neighbor_id = neighbor_it->second;
                auto voxel_it = m_voxels.find(neighbor_id);
                if (voxel_it != m_voxels.end()) {
                    Voxel& neighbor_voxel = voxel_it->second;
                    neighbor_voxel.SetFaceVisible(static_cast<Voxel::FaceIndex>((i ^ 1)), true);
                }
            }
        }

        // Remove voxel from data structures
        auto& transforms = m_voxel_transforms[voxel_to_remove.GetVoxelColor()];
        transforms.erase(std::remove(transforms.begin(), transforms.end(), voxel_to_remove.GetTransform()), transforms.end());

        auto& matrices = m_voxel_matrices[voxel_to_remove.GetVoxelColor()];
        matrices.erase(std::remove(matrices.begin(), matrices.end(), voxel_to_remove.GetTransform().GetTransform()), matrices.end());

        m_position_to_id.erase(grid_pos);
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
