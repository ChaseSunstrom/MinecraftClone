#include "scene.hpp"
#include "ray.hpp"
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <random>
#include <chrono>

namespace MC {

    // Constants for world generation
    const i32 CHUNK_LOAD_RADIUS = 32;
    const i32 CHUNK_LOAD_HEIGHT = 8;
    const f32 BIOME_SCALE = 0.0001f;
    const f32 ELEVATION_SCALE = 0.01f;
    const f32 CAVE_SCALE = 0.05f;
    const f32 TREE_SCALE = 0.03f;
    const f32 ORE_SCALE = 0.1f;
    const f32 CAVE_THRESHOLD = 0.6f;
    const f32 TREE_THRESHOLD = 0.8f;
    const i32 SEA_LEVEL = 65;

    f32 Hash(i32 x, i32 y, i32 z, uint32_t seed) {
        uint32_t h = seed;
        h ^= x + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= y + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= z + 0x9e3779b9 + (h << 6) + (h >> 2);
        return static_cast<f32>(h % 1000000) / 1000000.0f; // Normalize to [0,1)
    }

    f32 Hash(i32 x, i32 z, uint32_t seed) {
        uint32_t h = seed;
        h ^= x + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= z + 0x9e3779b9 + (h << 6) + (h >> 2);
        return static_cast<f32>(h % 1000000) / 1000000.0f; // Normalize to [0,1)
    }


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
        m_last_player_chunk_pos(glm::ivec3(std::numeric_limits<i32>::max())) // Initialize to an invalid position
    {
        Camera& camera = *m_camera;
        m_event_handler.SubscribeToEvent<WindowResizedEvent>([&camera](EventPtr<WindowResizedEvent> event) {
            camera.OnWindowResize(event);
            });

        std::random_device rd;
        std::mt19937 mt(rd());
        m_seed = mt();
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

        std::vector<glm::ivec3> chunks_to_load;

        for (i32 x = -CHUNK_LOAD_RADIUS; x <= CHUNK_LOAD_RADIUS; ++x) {
            for (i32 y = -CHUNK_LOAD_HEIGHT; y <= CHUNK_LOAD_HEIGHT; ++y) {
                for (i32 z = -CHUNK_LOAD_RADIUS; z <= CHUNK_LOAD_RADIUS; ++z) {
                    glm::ivec3 offset(x, y, z);
                    f32 distance = glm::length(glm::vec3(offset));
                    if (distance > CHUNK_LOAD_RADIUS) {
                        continue; // Skip chunks beyond the radius
                    }
                    glm::ivec3 chunk_pos = player_chunk_pos + offset;
                    if (m_chunks.find(chunk_pos) == m_chunks.end()) {
                        chunks_to_load.push_back(chunk_pos);
                    }
                }
            }
        }

        // Sort chunks_to_load based on distance
        std::sort(chunks_to_load.begin(), chunks_to_load.end(), [player_chunk_pos](const glm::ivec3& a, const glm::ivec3& b) {
            return glm::length(glm::vec3(a - player_chunk_pos)) < glm::length(glm::vec3(b - player_chunk_pos));
            });

        // Limit the number of chunks to generate per frame
        const size_t MAX_CHUNKS_PER_FRAME = 10;
        size_t chunks_loaded = 0;

        for (const auto& chunk_pos : chunks_to_load) {
            if (chunks_loaded >= MAX_CHUNKS_PER_FRAME) {
                break;
            }
            GenerateChunk(chunk_pos);
            ++chunks_loaded;
        }
    }

    void Scene::GenerateChunk(const glm::ivec3& chunk_pos) {
        m_chunks.emplace(
            chunk_pos,
            std::make_shared<Chunk>(chunk_pos)
        );

        auto chunk_it = m_chunks.find(chunk_pos);
        auto chunk = chunk_it->second;

        GenerateVoxelDataForChunk(*chunk);
        chunk->SetNeedsMeshUpdate(true);
    }

    BiomeType Scene::GetBiomeType(i32 world_x, i32 world_z) {
        f32 hash = Hash(world_x, world_z, m_seed);
        f32 biome_noise = glm::perlin(glm::vec2(world_x * BIOME_SCALE, world_z * BIOME_SCALE) + glm::vec2(hash));
        biome_noise = (biome_noise + 1.0f) / 2.0f;

        if (biome_noise < 0.1f) return BiomeType::OCEAN;
        if (biome_noise < 0.2f) return BiomeType::DESERT;
        if (biome_noise < 0.3f) return BiomeType::PLAINS;
        if (biome_noise < 0.4f) return BiomeType::FOREST;
        if (biome_noise < 0.5f) return BiomeType::SWAMP;
        if (biome_noise < 0.6f) return BiomeType::JUNGLE;
        if (biome_noise < 0.7f) return BiomeType::SAVANNA;
        if (biome_noise < 0.8f) return BiomeType::TAIGA;
        if (biome_noise < 0.9f) return BiomeType::MOUNTAINS;
        return BiomeType::SNOWY_MOUNTAINS;
    }

    f32 Scene::ComputeElevationNoise(f32 x, f32 z) {
        const i32 OCTAVES = 4;
        const f32 PERSISTENCE = 0.5f;
        const f32 LACUNARITY = 2.0f;
        f32 frequency = ELEVATION_SCALE;
        f32 amplitude = 1.0f;
        f32 max_amplitude = 0.0f;
        f32 noise_value = 0.0f;

        for (i32 i = 0; i < OCTAVES; ++i) {
            f32 noise = glm::perlin(glm::vec2(x * frequency, z * frequency)) * amplitude;
            noise_value += noise;
            max_amplitude += amplitude;

            amplitude *= PERSISTENCE;
            frequency *= LACUNARITY;
        }

        noise_value /= max_amplitude; // Normalize
        return (noise_value + 1.0f) / 2.0f; // Map to [0, 1]
    }


    i32 Scene::GetTerrainHeight(i32 world_x, i32 world_z, BiomeType biome) {
        f32 elevation = ComputeElevationNoise(static_cast<f32>(world_x), static_cast<f32>(world_z));

        switch (biome) {
        case BiomeType::PLAINS:
            return static_cast<i32>(elevation * 10 + 50);
        case BiomeType::MOUNTAINS:
            return static_cast<i32>(elevation * 40 + 80);
        case BiomeType::DESERT:
            return static_cast<i32>(elevation * 5 + 45);
        case BiomeType::FOREST:
            return static_cast<i32>(elevation * 15 + 55);
        case BiomeType::SWAMP:
            return static_cast<i32>(elevation * 4 + 48);
        case BiomeType::JUNGLE:
            return static_cast<i32>(elevation * 20 + 60);
        case BiomeType::SAVANNA:
            return static_cast<i32>(elevation * 12 + 52);
        case BiomeType::TAIGA:
            return static_cast<i32>(elevation * 18 + 58);
        case BiomeType::SNOWY_MOUNTAINS:
            return static_cast<i32>(elevation * 50 + 90);
        case BiomeType::OCEAN:
            return static_cast<i32>(elevation * -20 + 30);
        default:
            return static_cast<i32>(elevation * 20 + 60);
        }
    }


    bool Scene::IsCave(i32 world_x, i32 world_y, i32 world_z) {
        i32 hash = Hash(world_x, world_y, world_z, m_seed);
        f32 cave_noise = glm::perlin(glm::vec3(world_x * CAVE_SCALE, world_y * CAVE_SCALE, world_z * CAVE_SCALE) + glm::vec3(hash));
        return cave_noise > CAVE_THRESHOLD;
    }

    VoxelType Scene::GetVoxelType(i32 world_x, i32 world_y, i32 world_z, i32 terrain_height, BiomeType biome) {
        if (world_y > terrain_height) {
            if (biome == BiomeType::OCEAN && world_y <= SEA_LEVEL) {
                return VoxelType::WATER;
            }
            if (biome == BiomeType::SWAMP && world_y == terrain_height + 1) {
                return VoxelType::WATER;
            }
            // Snow on top of snowy mountains
            if (biome == BiomeType::SNOWY_MOUNTAINS && world_y <= terrain_height + 1) {
                return VoxelType::SNOW;
            }
            return VoxelType::AIR;
        }

        // Check for caves
        if (world_y < terrain_height && IsCave(world_x, world_y, world_z)) {
            return VoxelType::AIR;
        }

        // Bedrock layer at the bottom
        if (world_y == 0) {
            return VoxelType::BEDROCK;
        }

        // Ore generation
        if (world_y < 60 && world_y > 5) {
            f32 ore_noise = glm::perlin(glm::vec3(world_x * ORE_SCALE, world_y * ORE_SCALE, world_z * ORE_SCALE));
            ore_noise = (ore_noise + 1.0f) / 2.0f;
            if (ore_noise > 0.8f) {
                return VoxelType::COAL_ORE;
            }
            else if (ore_noise > 0.85f) {
                return VoxelType::IRON_ORE;
            }
            else if (ore_noise > 0.9f) {
                return VoxelType::GOLD_ORE;
            }
            else if (ore_noise > 0.95f) {
                return VoxelType::DIAMOND_ORE;
            }
        }

        // Determine voxel type based on depth
        i32 depth = terrain_height - world_y;

        if (depth == 0) {
            // Surface block
            switch (biome) {
            case BiomeType::DESERT:
                return VoxelType::SAND;
            case BiomeType::FOREST:
                return VoxelType::GRASS_FOREST;
            case BiomeType::PLAINS:
                return VoxelType::GRASS_PLAINS;
            case BiomeType::JUNGLE:
                return VoxelType::GRASS_JUNGLE;
            case BiomeType::SAVANNA:
                return VoxelType::GRASS_SAVANNA;
            case BiomeType::TAIGA:
                return VoxelType::GRASS_TAIGA;
            case BiomeType::SWAMP:
                return VoxelType::DIRT;
            case BiomeType::MOUNTAINS:
                return VoxelType::STONE;
            case BiomeType::SNOWY_MOUNTAINS:
                return VoxelType::STONE;
            default:
                return VoxelType::GRASS_PLAINS;
            }
        }
        else if (depth < 5) {
            // Sub-surface block
            if (biome == BiomeType::DESERT) {
                return VoxelType::SAND;
            }
            return VoxelType::DIRT;
        }
        else {
            // Underground blocks
            return VoxelType::STONE;
        }
    }

    void Scene::GenerateTrees(Chunk& chunk, i32 world_x, i32 world_z, i32 terrain_height, BiomeType biome) {
        f32 hash = Hash(world_x, world_z, m_seed);
        f32 tree_noise = glm::perlin(glm::vec2(world_x * TREE_SCALE, world_z * TREE_SCALE) + glm::vec2(hash));

        if (tree_noise > TREE_THRESHOLD) {
            // Place a tree at this location
            i32 trunk_height = 5;

            // Adjust trunk height for different biomes
            if (biome == BiomeType::JUNGLE) {
                trunk_height = 10;
            }
            else if (biome == BiomeType::TAIGA) {
                trunk_height = 7;
            }

            // Generate the trunk
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
                            VoxelType leaf_type = VoxelType::LEAVES;

                            // For snowy biomes, use snow instead of leaves
                            if (biome == BiomeType::SNOWY_MOUNTAINS || biome == BiomeType::TAIGA) {
                                leaf_type = VoxelType::SNOW;
                            }

                            Voxel voxel(leaf_type);
                            chunk.SetVoxel(local_pos, voxel);
                        }
                    }
                }
            }
        }
    }

    void Scene::GenerateVoxelDataForChunk(Chunk& chunk) {
        glm::ivec3 chunk_pos = chunk.GetPosition();

        std::vector<BiomeType> biome_types(Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE);
        std::vector<i32> terrain_heights(Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE);

        // Precompute biome types and terrain heights
        for (i32 x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (i32 z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                i32 world_x = chunk_pos.x * Chunk::CHUNK_SIZE + x;
                i32 world_z = chunk_pos.z * Chunk::CHUNK_SIZE + z;
                size_t index = x * Chunk::CHUNK_SIZE + z;

                biome_types[index] = GetBiomeType(world_x, world_z);
                terrain_heights[index] = GetTerrainHeight(world_x, world_z, biome_types[index]);
            }
        }

        // Generate voxels using precomputed data
        for (i32 x = 0; x < Chunk::CHUNK_SIZE; ++x) {
            for (i32 z = 0; z < Chunk::CHUNK_SIZE; ++z) {
                i32 world_x = chunk_pos.x * Chunk::CHUNK_SIZE + x;
                i32 world_z = chunk_pos.z * Chunk::CHUNK_SIZE + z;
                size_t index = x * Chunk::CHUNK_SIZE + z;
                BiomeType biome = biome_types[index];
                i32 terrain_height = terrain_heights[index];

                for (i32 y = 0; y < Chunk::CHUNK_SIZE; ++y) {
                    i32 world_y = chunk_pos.y * Chunk::CHUNK_SIZE + y;
                    VoxelType voxel_type = GetVoxelType(world_x, world_y, world_z, terrain_height, biome);

                    if (voxel_type != VoxelType::AIR) {
                        glm::ivec3 local_pos(x, y, z);
                        Voxel voxel(voxel_type);
                        chunk.SetVoxel(local_pos, voxel);
                    }
                }

                // Generate trees if necessary
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
            if (chunk->NeedsMeshUpdate() || !chunk->IsMeshDataUploaded()) {
                chunk->Update(*this, m_thread_pool);
            }
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
