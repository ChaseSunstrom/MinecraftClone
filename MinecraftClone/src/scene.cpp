#include "scene.hpp"
#include "ray.hpp"
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <random>
#include <chrono>

namespace MC {

    // Constants for world generation
    const i32 CHUNK_LOAD_RADIUS = 16;
    const i32 CHUNK_LOAD_HEIGHT = 4;
    const f32 BIOME_SCALE = 0.001f;
    const f32 ELEVATION_SCALE = 0.01f;
    const f32 CAVE_SCALE = 0.05f;
    const f32 TREE_SCALE = 0.02f;
    const f32 ORE_SCALE = 0.1f;
    const f32 CAVE_THRESHOLD = 0.6f;
    const f32 TREE_THRESHOLD = 0.8f;

    // Helper function to convert world position to chunk and local positions
    void WorldToChunkLocal(const glm::ivec3& worldPos, glm::ivec3& chunk_pos, glm::ivec3& local_pos) {
        chunk_pos.x = static_cast<i32>(std::floor(worldPos.x / static_cast<f32>(Chunk::CHUNK_SIZE)));
        chunk_pos.y = static_cast<i32>(std::floor(worldPos.y / static_cast<f32>(Chunk::CHUNK_SIZE)));
        chunk_pos.z = static_cast<i32>(std::floor(worldPos.z / static_cast<f32>(Chunk::CHUNK_SIZE)));

        local_pos = worldPos - chunk_pos * Chunk::CHUNK_SIZE;
    }

    Scene::Scene(EventHandler& event_handler, ThreadPool& tp)
        : m_event_handler(event_handler), m_thread_pool(tp),
        m_camera(std::make_unique<Camera>()),
        m_sky_color(glm::vec4(0.2f, 0.3f, 0.4f, 1.0f)),
        m_last_player_chunk_pos(m_camera->GetPosition())
    {
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
        UpdateChunksAroundPlayer();
    }

    void Scene::UpdateChunksAroundPlayer() {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        glm::vec3 player_pos = m_camera->GetPosition();
        glm::ivec3 player_chunk_pos = glm::floor(player_pos / static_cast<f32>(Chunk::CHUNK_SIZE));

        if (player_chunk_pos == m_last_player_chunk_pos) {
            // Player hasn't moved to a new chunk, no need to update
            return;
        }

        m_last_player_chunk_pos = player_chunk_pos;

        // Store chunks to unload
        std::vector<glm::ivec3> chunks_to_unload;

        // Identify chunks to unload
        for (const auto& [chunk_pos, chunk] : m_chunks) {
            if (glm::distance(glm::vec3(chunk_pos), glm::vec3(player_chunk_pos)) > CHUNK_LOAD_RADIUS) {
                chunks_to_unload.push_back(chunk_pos);
            }
        }

        // Unload chunks
        for (const auto& chunk_pos : chunks_to_unload) {
            m_chunks.erase(chunk_pos);
        }

        // Load new chunks
        for (i32 x = -CHUNK_LOAD_RADIUS; x <= CHUNK_LOAD_RADIUS; ++x) {
            for (i32 y = -CHUNK_LOAD_HEIGHT; y <= CHUNK_LOAD_HEIGHT; ++y) {
                for (i32 z = -CHUNK_LOAD_RADIUS; z <= CHUNK_LOAD_RADIUS; ++z) {
                    glm::ivec3 offset(x, y, z);
                    if (glm::length(glm::vec3(offset)) > CHUNK_LOAD_RADIUS) {
                        continue; // Skip chunks beyond the radius
                    }
                    glm::ivec3 chunk_pos = player_chunk_pos + offset;
                    if (m_chunks.find(chunk_pos) == m_chunks.end()) {
                        GenerateChunk(chunk_pos);
                    }
                }
            }
        }

    }

    void Scene::GenerateChunk(const glm::ivec3& chunk_pos) {
        m_chunks.emplace(
            chunk_pos,
            std::make_shared<Chunk>(chunk_pos)
        );

        auto chunk_it = m_chunks.find(chunk_pos);
        auto chunk = chunk_it->second;

        // Enqueue voxel data generation in the thread pool
        m_thread_pool.Enqueue(TaskPriority::VERY_HIGH, false, [this, chunk]() {
                GenerateVoxelDataForChunk(*chunk);
                chunk->SetNeedsMeshUpdate(true);
            });
    }

    BiomeType Scene::GetBiomeType(i32 world_x, i32 world_z) {
        f32 biome_noise = glm::perlin(glm::vec2(world_x * BIOME_SCALE, world_z * BIOME_SCALE));
        biome_noise = (biome_noise + 1.0f) / 2.0f;

        if (biome_noise < 0.2f) return BiomeType::DESERT;
        if (biome_noise < 0.4f) return BiomeType::PLAINS;
        if (biome_noise < 0.6f) return BiomeType::FOREST;
        if (biome_noise < 0.8f) return BiomeType::SWAMP;
        return BiomeType::MOUNTAINS;
    }

    i32 Scene::GetTerrainHeight(i32 world_x, i32 world_z, BiomeType biome) {
        f32 elevation = glm::perlin(glm::vec2(world_x * ELEVATION_SCALE, world_z * ELEVATION_SCALE));
        elevation = (elevation + 1.0f) / 2.0f;

        switch (biome) {
        case BiomeType::PLAINS:
            return static_cast<i32>(elevation * 10 + 50);
        case BiomeType::MOUNTAINS:
            return static_cast<i32>(elevation * 30 + 70);
        case BiomeType::DESERT:
            return static_cast<i32>(elevation * 5 + 45);
        case BiomeType::FOREST:
            return static_cast<i32>(elevation * 15 + 55);
        case BiomeType::SWAMP:
            return static_cast<i32>(elevation * 8 + 48);
        default:
            return static_cast<i32>(elevation * 20 + 60);
        }
    }

    bool Scene::IsCave(i32 world_x, i32 world_y, i32 world_z) {
        f32 cave_noise = glm::perlin(glm::vec3(world_x * CAVE_SCALE, world_y * CAVE_SCALE, world_z * CAVE_SCALE));
        return cave_noise > CAVE_THRESHOLD;
    }

    VoxelType Scene::GetVoxelType(i32 world_x, i32 world_y, i32 world_z, i32 terrain_height, BiomeType biome) {
        if (world_y > terrain_height) {
            return VoxelType::AIR;
        }

        // Check for caves
        if (world_y < terrain_height && IsCave(world_x, world_y, world_z)) {
            return VoxelType::AIR;
        }

        // Determine voxel type based on depth
        i32 depth = terrain_height - world_y;

        if (depth == 0) {
            // Surface block
            switch (biome) {
            case BiomeType::DESERT:
                return VoxelType::SAND;
            case BiomeType::FOREST:
            case BiomeType::PLAINS:
                return VoxelType::GRASS;
            case BiomeType::SWAMP:
                return VoxelType::DIRT;
            case BiomeType::MOUNTAINS:
                return VoxelType::STONE;
            default:
                return VoxelType::GRASS;
            }
        }
        else if (depth < 5) {
            // Sub-surface block
            return VoxelType::DIRT;
        }
        else {
            // Underground blocks
            return VoxelType::STONE;
        }

        // Add ore generation logic here if desired
    }

    void Scene::GenerateTrees(Chunk& chunk, i32 world_x, i32 world_z, i32 terrain_height, BiomeType biome) {
        if (biome == BiomeType::FOREST) {
            f32 tree_noise = glm::perlin(glm::vec2(world_x * TREE_SCALE, world_z * TREE_SCALE));
            if (tree_noise > TREE_THRESHOLD) {
                // Place a tree at this location
                // Simple tree structure
                i32 trunk_height = 5;
                for (i32 y = 1; y <= trunk_height; ++y) {
                    i32 world_y = terrain_height + y;
                    glm::ivec3 local_pos = glm::ivec3(
                        world_x % Chunk::CHUNK_SIZE,
                        world_y % Chunk::CHUNK_SIZE,
                        world_z % Chunk::CHUNK_SIZE
                    );
                    Voxel voxel(VoxelType::WOOD);
                    chunk.SetVoxel(local_pos, voxel);
                }
                // Add leaves
                i32 leaf_start = terrain_height + trunk_height - 2;
                for (i32 y = leaf_start; y <= terrain_height + trunk_height + 1; ++y) {
                    for (i32 dx = -2; dx <= 2; ++dx) {
                        for (i32 dz = -2; dz <= 2; ++dz) {
                            if (dx * dx + dz * dz <= 4) {
                                i32 world_y = y;
                                i32 world_leaf_x = world_x + dx;
                                i32 world_leaf_z = world_z + dz;
                                glm::ivec3 local_pos = glm::ivec3(
                                    world_leaf_x % Chunk::CHUNK_SIZE,
                                    world_y % Chunk::CHUNK_SIZE,
                                    world_leaf_z % Chunk::CHUNK_SIZE
                                );
                                Voxel voxel(VoxelType::LEAVES);
                                chunk.SetVoxel(local_pos, voxel);
                            }
                        }
                    }
                }
            }
        }
    }

    void Scene::GenerateVoxelDataForChunk(Chunk& chunk) {
        glm::ivec3 chunk_pos = chunk.GetPosition();

        for (i32 x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (i32 z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                i32 world_x = chunk_pos.x * Chunk::CHUNK_SIZE + x;
                i32 world_z = chunk_pos.z * Chunk::CHUNK_SIZE + z;

                BiomeType biome = GetBiomeType(world_x, world_z);
                i32 terrain_height = GetTerrainHeight(world_x, world_z, biome);

                for (i32 y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                    i32 world_y = chunk_pos.y * Chunk::CHUNK_SIZE + y;

                    VoxelType voxel_type = GetVoxelType(world_x, world_y, world_z, terrain_height, biome);

                    if (voxel_type != VoxelType::AIR) {
                        glm::ivec3 local_pos(x, y, z);
                        Voxel voxel(voxel_type);
                        chunk.SetVoxel(local_pos, voxel);
                    }
                }

                // Generate trees and other vegetation
                if (chunk_pos.y == terrain_height / Chunk::CHUNK_SIZE) {
                    GenerateTrees(chunk, world_x, world_z, terrain_height % Chunk::CHUNK_SIZE, biome);
                }
            }
        }
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
                chunk_pos,
                std::make_shared<Chunk>(chunk_pos)
            );
            chunk_it = m_chunks.find(chunk_pos);
        }

        Chunk& chunk = *chunk_it->second;

        // Check for existing voxel at the position
        auto existing_voxel = chunk.GetVoxel(local_pos);
        if (existing_voxel.has_value()) {
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

        auto chunk_it = m_chunks.find(chunk_pos);
        if (chunk_it != m_chunks.end()) {
            Chunk& chunk = *chunk_it->second;
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

        auto chunk_it = m_chunks.find(chunk_pos);
        if (chunk_it != m_chunks.end()) {
            const Chunk& chunk = *chunk_it->second;
            return chunk.GetVoxel(local_pos);
        }
        return std::nullopt;
    }

    std::optional<Voxel> Scene::GetVoxelAtPosition(const glm::ivec3& worldPos) const {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        glm::ivec3 chunk_pos, local_pos;
        WorldToChunkLocal(worldPos, chunk_pos, local_pos);

        auto chunk_it = m_chunks.find(chunk_pos);
        if (chunk_it != m_chunks.end()) {
            const Chunk& chunk = *chunk_it->second;
            return chunk.GetVoxel(local_pos);
        }
        return std::nullopt;
    }

    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>>& Scene::GetChunks() {
        return m_chunks;
    }

    Camera& Scene::GetCamera() const {
        return *m_camera;
    }

    void Scene::UpdateChunks() {
        for (auto& [chunk_pos, chunk] : m_chunks) {
            chunk->Update(*this, m_thread_pool);
        }
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
