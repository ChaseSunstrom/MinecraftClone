#include "scene.hpp"
#include "ray.hpp"
#include <cmath>

namespace MC {
    // Helper function to convert world position to chunk and local positions
    void WorldToChunkLocal(const glm::ivec3& worldPos, glm::ivec3& chunk_pos, glm::ivec3& local_pos) {
        chunk_pos.x = std::floor(worldPos.x / (f32)Chunk::CHUNK_SIZE);
        chunk_pos.y = std::floor(worldPos.y / (f32)Chunk::CHUNK_SIZE);
        chunk_pos.z = std::floor(worldPos.z / (f32)Chunk::CHUNK_SIZE);

        local_pos = worldPos - chunk_pos * Chunk::CHUNK_SIZE;
    }

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
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        glm::vec3 pos = voxel.GetTransform().GetPos();
        glm::ivec3 worldPos = glm::ivec3(glm::floor(pos));

        glm::ivec3 chunk_pos, local_pos;
        WorldToChunkLocal(worldPos, chunk_pos, local_pos);

        // Get or create the chunk
        auto chunk_it = m_chunks.find(chunk_pos);
        if (chunk_it == m_chunks.end()) {
            m_chunks.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(chunk_pos),
                std::forward_as_tuple(chunk_pos)
            );
            chunk_it = m_chunks.find(chunk_pos);
        }

        Chunk& chunk = chunk_it->second;

        // Check for existing voxel at the position
        auto existingVoxel = chunk.GetVoxel(local_pos);
        if (existingVoxel.has_value()) {
            std::cerr << "Cannot insert voxel: A voxel already exists at ("
                << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ").\n";
            return;
        }

        // Insert the voxel
        chunk.SetVoxel(local_pos, voxel);
        m_voxelLocations[voxel.GetID()] = std::make_pair(chunk_pos, local_pos);
    }

    void Scene::InsertVoxels(const std::vector<Voxel>& voxels) {
        for (const auto& voxel : voxels) {
            InsertVoxel(voxel);
        }
    }

    void Scene::RemoveVoxel(u32 voxel_id) {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        auto voxelLocIt = m_voxelLocations.find(voxel_id);
        if (voxelLocIt == m_voxelLocations.end()) {
            std::cerr << "Attempted to remove non-existent voxel with ID: " << voxel_id << "\n";
            return;
        }

        glm::ivec3 chunk_pos = voxelLocIt->second.first;
        glm::ivec3 local_pos = voxelLocIt->second.second;

        auto chunkIt = m_chunks.find(chunk_pos);
        if (chunkIt != m_chunks.end()) {
            Chunk& chunk = chunkIt->second;
            chunk.RemoveVoxel(local_pos);
            m_voxelLocations.erase(voxelLocIt);
        }
    }

    std::optional<Voxel> Scene::GetVoxel(u32 id) const {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        auto voxel_loc_it = m_voxelLocations.find(id);
        if (voxel_loc_it == m_voxelLocations.end()) {
            return std::nullopt;
        }

        glm::ivec3 chunk_pos = voxel_loc_it->second.first;
        glm::ivec3 local_pos = voxel_loc_it->second.second;

        auto chunkIt = m_chunks.find(chunk_pos);
        if (chunkIt != m_chunks.end()) {
            const Chunk& chunk = chunkIt->second;
            return chunk.GetVoxel(local_pos);
        }
        return std::nullopt;
    }

    std::optional<Voxel> Scene::GetVoxelAtPosition(const glm::ivec3& worldPos) const {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        glm::ivec3 chunk_pos, local_pos;
        WorldToChunkLocal(worldPos, chunk_pos, local_pos);

        auto chunkIt = m_chunks.find(chunk_pos);
        if (chunkIt != m_chunks.end()) {
            const Chunk& chunk = chunkIt->second;
            return chunk.GetVoxel(local_pos);
        }
        return std::nullopt;
    }

    std::unordered_map<glm::ivec3, Chunk>& Scene::GetChunks() {
        return m_chunks;
    }

    Camera& Scene::GetCamera() const {
        return *m_camera;
    }

    std::optional<VoxelHitInfo> Scene::GetVoxelLookedAt(f32 max_distance) const {
        Camera& camera = GetCamera();
        glm::vec3 origin = camera.GetPosition();
        glm::vec3 direction = glm::normalize(camera.GetFront());

        Ray ray(origin, direction);
        glm::ivec3 current_voxel = glm::ivec3(glm::floor(ray.origin));

        glm::vec3 t_max;
        glm::vec3 t_delta;
        glm::ivec3 step;
        i32 last_step_axis = -1;

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
                    hit_face = VoxelFace::POS_X;
                }

                VoxelHitInfo hit_info{ voxel_opt.value(), hit_face };
                return hit_info;
            }

            // Determine which axis to step
            if (t_max.x < t_max.y) {
                if (t_max.x < t_max.z) {
                    current_voxel.x += step.x;
                    distance_traveled = t_max.x;
                    t_max.x += t_delta.x;
                    last_step_axis = 0;
                }
                else {
                    current_voxel.z += step.z;
                    distance_traveled = t_max.z;
                    t_max.z += t_delta.z;
                    last_step_axis = 2;
                }
            }
            else {
                if (t_max.y < t_max.z) {
                    current_voxel.y += step.y;
                    distance_traveled = t_max.y;
                    t_max.y += t_delta.y;
                    last_step_axis = 1;
                }
                else {
                    current_voxel.z += step.z;
                    distance_traveled = t_max.z;
                    t_max.z += t_delta.z;
                    last_step_axis = 2;
                }
            }
        }

        return std::nullopt;
    }

}
