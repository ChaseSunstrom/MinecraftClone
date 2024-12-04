#include "scene.hpp"
#include "ray.hpp"
#include <FastNoise/FastNoise.h>
#include <glm/gtc/noise.hpp>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>

namespace MC {
    // Constants for world generation
    const i32 CHUNK_LOAD_RADIUS = 32;
    const i32 CHUNK_LOAD_HEIGHT = 4;
    const f32 BIOME_SCALE = 0.003f; // Larger scale for smaller biomes
    const f32 ELEVATION_SCALE = 0.05f; // Smaller scale for smoother terrain
    const f32 CAVE_SCALE = 0.05f;
    const f32 TREE_SCALE = 0.03f;
    const f32 ORE_SCALE = 0.1f;
    const f32 CAVE_THRESHOLD = 0.6f;
    const f32 TREE_THRESHOLD = 0.8f;
    const i32 SEA_LEVEL = 60;
    // Limit the number of chunks to generate per frame
    const size_t MAX_CHUNKS_PER_FRAME = 2;

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
    void WorldToChunkLocal(const glm::ivec3& world_pos, glm::ivec3& chunk_pos, glm::ivec3& local_pos) {
        chunk_pos = glm::floor(glm::vec3(world_pos) / static_cast<f32>(Chunk::CHUNK_SIZE));

        local_pos = world_pos - chunk_pos * Chunk::CHUNK_SIZE;
        local_pos = (local_pos % Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE) % Chunk::CHUNK_SIZE;
    }

    Scene::Scene(EventHandler& event_handler, ThreadPool& tp)
        : m_event_handler(event_handler), m_thread_pool(tp),
        m_camera(std::make_unique<Camera>()),
        m_sky_color(glm::vec4(0.2f, 0.3f, 0.4f, 1.0f)),
        m_last_player_chunk_pos(glm::ivec3(std::numeric_limits<i32>::max()))
    {
        Camera& camera = *m_camera;
        m_event_handler.SubscribeToEvent<WindowResizedEvent>([&camera](EventPtr<WindowResizedEvent> event) {
            camera.OnWindowResize(event);
            });

        std::random_device rd;
        std::mt19937 mt(rd());
        m_seed = mt();

        // Initialize Elevation Noise
        m_elevation_generator = FastNoise::New<FastNoise::Perlin>();
        m_elevation_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_elevation_fractal->SetSource(m_elevation_generator);
        m_elevation_fractal->SetOctaveCount(6); // Increased octaves
        //m_elevation_fractal->SetGain(0.4f); // Persistence
        //m_elevation_fractal->SetLacunarity(2.2f); // Lacunarity

        // Initialize Biome Noise
        m_biome_generator = FastNoise::New<FastNoise::Perlin>();
        m_biome_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_biome_fractal->SetSource(m_biome_generator);
        m_biome_fractal->SetOctaveCount(4);
        //m_biome_fractal->SetGain(0.5f);
        //m_biome_fractal->SetLacunarity(2.0f);

        // Initialize Cave Noise
        m_cave_generator = FastNoise::New<FastNoise::Perlin>();
        m_cave_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_cave_fractal->SetSource(m_cave_generator);
        m_cave_fractal->SetOctaveCount(3);
        //m_cave_fractal->SetGain(0.6f);
        //m_cave_fractal->SetLacunarity(2.0f);

        // Initialize Tree Noise
        m_tree_generator = FastNoise::New<FastNoise::Perlin>();
        m_tree_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_tree_fractal->SetSource(m_tree_generator);
        m_tree_fractal->SetOctaveCount(2);
        //m_tree_fractal->SetGain(0.5f);
        //m_tree_fractal->SetLacunarity(2.0f);

        // Initialize Ore Noise
        m_ore_generator = FastNoise::New<FastNoise::Perlin>();
        m_ore_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_ore_fractal->SetSource(m_ore_generator);
        m_ore_fractal->SetOctaveCount(3);
        //m_ore_fractal->SetGain(0.7f);
        //m_ore_fractal->SetLacunarity(2.0f);

        // Initialize Temperature Noise
        m_temperature_generator = FastNoise::New<FastNoise::Perlin>();
        m_temperature_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_temperature_fractal->SetSource(m_temperature_generator);
        m_temperature_fractal->SetOctaveCount(4);
        //m_temperature_fractal->SetGain(0.5f);
        //m_temperature_fractal->SetLacunarity(2.0f);

        // Initialize Humidity Noise
        m_humidity_generator = FastNoise::New<FastNoise::Perlin>();
        m_humidity_fractal = FastNoise::New<FastNoise::FractalFBm>();
        m_humidity_fractal->SetSource(m_humidity_generator);
        m_humidity_fractal->SetOctaveCount(4);
        //m_humidity_fractal->SetGain(0.5f);
        //m_humidity_fractal->SetLacunarity(2.0f);
    }

    Scene::~Scene() {
        Voxel::CleanupStaticBuffers();
    }

    void Scene::InitializeScene() {
        Voxel::InitializeStaticBuffers();
        m_sun.Initialize();
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
        auto new_chunk = std::make_shared<Chunk>(chunk_pos);
        m_chunks.emplace(chunk_pos, new_chunk);

        GenerateVoxelDataForChunk(*new_chunk);
        new_chunk->SetNeedsMeshUpdate(true);
    }

    BiomeType Scene::GetBiomeType(i32 world_x, i32 world_z) {
        // Generate temperature and humidity values
        f32 temperature = m_temperature_fractal->GenSingle2D(world_x * BIOME_SCALE, world_z * BIOME_SCALE, m_seed + 6);
        temperature = (temperature + 1.0f) / 2.0f; // Normalize to [0,1]

        f32 humidity = m_humidity_fractal->GenSingle2D(world_x * BIOME_SCALE, world_z * BIOME_SCALE, m_seed + 7);
        humidity = (humidity + 1.0f) / 2.0f; // Normalize to [0,1]

        // Determine biome based on temperature and humidity
        if (temperature < 0.2f) {
            if (humidity < 0.3f) return BiomeType::DESERT;
            else if (humidity < 0.6f) return BiomeType::SAVANNA;
            else return BiomeType::MESA;
        }
        else if (temperature < 0.4f) {
            if (humidity < 0.3f) return BiomeType::SAVANNA;
            else if (humidity < 0.6f) return BiomeType::FOREST;
            else return BiomeType::MANGROVE;
        }
        else if (temperature < 0.6f) {
            if (humidity < 0.3f) return BiomeType::TAIGA;
            else if (humidity < 0.6f) return BiomeType::BIRCH_FOREST;
            else return BiomeType::SWAMP;
        }
        else if (temperature < 0.8f) {
            if (humidity < 0.3f) return BiomeType::FOREST;
            else if (humidity < 0.6f) return BiomeType::JUNGLE;
            else return BiomeType::SWAMP;
        }
        else {
            if (humidity < 0.3f) return BiomeType::SNOWY_MOUNTAINS;
            else if (humidity < 0.6f) return BiomeType::TUNDRA;
            else return BiomeType::SNOWY_MOUNTAINS;
        }

        // Fallback to PLAINS
        return BiomeType::PLAINS;
    }

    i32 Scene::GetBiomeElevation(f32 elevation, BiomeType biome) {
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
            return SEA_LEVEL - static_cast<i32>(elevation * 10); // Deeper oceans
        case BiomeType::TUNDRA:
            return static_cast<i32>(elevation * 8 + 55);
        case BiomeType::BIRCH_FOREST:
            return static_cast<i32>(elevation * 15 + 55);
        case BiomeType::MANGROVE:
            return static_cast<i32>(elevation * 12 + 50);
        case BiomeType::MESA:
            return static_cast<i32>(elevation * 6 + 48);
        default:
            return static_cast<i32>(elevation * 20 + 60);
        }
    }

    f32 Scene::ComputeElevationNoise(f32 x, f32 z) {
        // Minecraft-like elevation noise parameters
        const i32 OCTAVES = 6; // Increased for more detail
        const f32 PERSISTENCE = 0.4f; // Lower persistence for smoother terrain
        const f32 LACUNARITY = 2.2f; // Higher lacunarity for more frequency variation
        f32 frequency = ELEVATION_SCALE;
        f32 amplitude = 1.0f;
        f32 max_amplitude = 0.0f;
        f32 noise_value = 0.0f;

        for (i32 i = 0; i < OCTAVES; ++i) {
            // Generate noise using FastNoise
            f32 noise = m_elevation_fractal->GenSingle2D(x * frequency, z * frequency, m_seed + 1) * amplitude;
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

        // Adjust elevation based on biome
        switch (biome) {
        case BiomeType::MOUNTAINS:
        case BiomeType::SNOWY_MOUNTAINS:
            elevation *= 1.5f; // Higher elevations
            break;
        case BiomeType::DESERT:
            elevation *= 0.8f; // Smoother, lower elevations
            break;
        case BiomeType::JUNGLE:
        case BiomeType::FOREST:
            elevation *= 1.0f; // Standard
            break;
        case BiomeType::TUNDRA:
            elevation *= 1.2f;
            break;
        case BiomeType::MESA:
            elevation *= 0.9f;
            break;
            // Add more cases as needed
        default:
            elevation *= 1.0f;
        }

        // Get neighboring biome types and elevations
        struct Neighbor {
            BiomeType biome;
            i32 height;
        };

        std::vector<Neighbor> neighbors = {
            { biome, GetBiomeElevation(elevation, biome) },
            { GetBiomeType(world_x + 1, world_z), GetBiomeElevation(elevation, GetBiomeType(world_x + 1, world_z)) },
            { GetBiomeType(world_x - 1, world_z), GetBiomeElevation(elevation, GetBiomeType(world_x - 1, world_z)) },
            { GetBiomeType(world_x, world_z + 1), GetBiomeElevation(elevation, GetBiomeType(world_x, world_z + 1)) },
            { GetBiomeType(world_x, world_z - 1), GetBiomeElevation(elevation, GetBiomeType(world_x, world_z - 1)) },
        };

        // Calculate weights based on biome similarity (e.g., same biome gets higher weight)
        f32 total_weight = 0.0f;
        f32 blended_height = 0.0f;

        for (const auto& neighbor : neighbors) {
            f32 weight = (neighbor.biome == biome) ? 2.0f : 1.0f;
            blended_height += neighbor.height * weight;
            total_weight += weight;
        }

        blended_height /= total_weight;

        return static_cast<i32>(blended_height);
    }

    bool Scene::IsCave(i32 world_x, i32 world_y, i32 world_z) {
        f32 cave_noise = m_cave_fractal->GenSingle3D(world_x * CAVE_SCALE, world_y * CAVE_SCALE, world_z * CAVE_SCALE, m_seed + 3);
        return cave_noise > CAVE_THRESHOLD;
    }

    VoxelType Scene::GetVoxelType(i32 world_x, i32 world_y, i32 world_z, i32 terrain_height, BiomeType biome) {
        // Above terrain height
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
            // Ice near water bodies in cold biomes
            if ((biome == BiomeType::SNOWY_MOUNTAINS || biome == BiomeType::TAIGA || biome == BiomeType::TUNDRA) && world_y <= terrain_height + 2) {
                // Check if the voxel below is water
                VoxelType below_voxel = GetVoxelAtPosition({ world_x, world_y - 1, world_z });
                if (below_voxel == VoxelType::WATER) {
                    return VoxelType::ICE;
                }
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

        // Handle lava pools
        if (world_y < 10) { // Arbitrary threshold for lava depth
            f32 lava_noise = m_ore_fractal->GenSingle3D(world_x * ORE_SCALE, world_y * ORE_SCALE, world_z * ORE_SCALE, m_seed + 8);
            lava_noise = (lava_noise + 1.0f) / 2.0f; // Normalize to [0,1]
            if (lava_noise > 0.98f) { // Adjust threshold as needed
                return VoxelType::LAVA;
            }
        }

        // Generate ores
        if (world_y < 60 && world_y > 5) {
            f32 ore_noise = m_ore_fractal->GenSingle3D(world_x * ORE_SCALE, world_y * ORE_SCALE, world_z * ORE_SCALE, m_seed + 4);
            ore_noise = (ore_noise + 1.0f) / 2.0f;
            if (ore_noise > 0.96f) {
                return VoxelType::DIAMOND_ORE;
            }
            else if (ore_noise > 0.92f) {
                return VoxelType::GOLD_ORE;
            }
            else if (ore_noise > 0.88f) {
                return VoxelType::IRON_ORE;
            }
            else if (ore_noise > 0.84f) {
                return VoxelType::COAL_ORE;
            }
        }

        // Generate gravel in specific biomes or conditions
        if (biome == BiomeType::DESERT || biome == BiomeType::SAVANNA || biome == BiomeType::MESA) {
            f32 gravel_noise = m_biome_fractal->GenSingle2D(world_x * BIOME_SCALE, world_z * BIOME_SCALE, m_seed + 9);
            gravel_noise = (gravel_noise + 1.0f) / 2.0f;
            if (gravel_noise > 0.95f && world_y <= GetBiomeElevation(ComputeElevationNoise(static_cast<f32>(world_x), static_cast<f32>(world_z)), biome)) {
                return VoxelType::GRAVEL;
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
                return VoxelType::SNOW;
            case BiomeType::TUNDRA:
                return VoxelType::SNOW;
            case BiomeType::BIRCH_FOREST:
                return VoxelType::GRASS_BIRCH;
            case BiomeType::MANGROVE:
                return VoxelType::MANGROVE_WOOD;
            case BiomeType::MESA:
                return VoxelType::RED_SAND;
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
        f32 tree_noise = m_tree_fractal->GenSingle2D(world_x * TREE_SCALE, world_z * TREE_SCALE, m_seed + 5);
        tree_noise = (tree_noise + 1.0f) / 2.0f;

        if (tree_noise > TREE_THRESHOLD) {
            // Place a tree at this location
            i32 trunk_height = 5;

            // Adjust trunk height for different biomes
            switch (biome) {
            case BiomeType::JUNGLE:
                trunk_height = 10;
                break;
            case BiomeType::TAIGA:
                trunk_height = 7;
                break;
            case BiomeType::BIRCH_FOREST:
                trunk_height = 6;
                break;
            case BiomeType::MANGROVE:
                trunk_height = 8;
                break;
                // Add more cases as needed
            default:
                trunk_height = 5;
                break;
            }

            // Ensure trees don't exceed chunk boundaries
            i32 world_y = terrain_height + 1;
            for (i32 y = 0; y < trunk_height; ++y) {
                if (world_y + y >= CHUNK_LOAD_HEIGHT * Chunk::CHUNK_SIZE) break;
                glm::ivec3 local_pos = glm::ivec3(
                    (world_x % Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE) % Chunk::CHUNK_SIZE,
                    (world_y + y) % Chunk::CHUNK_SIZE,
                    (world_z % Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE) % Chunk::CHUNK_SIZE
                );
                chunk.SetVoxel(local_pos, VoxelType::WOOD);
            }

            // Add leaves with better distribution
            i32 leaf_start = terrain_height + trunk_height - 2;
            for (i32 y = leaf_start; y <= terrain_height + trunk_height + 1; ++y) {
                for (i32 dx = -2; dx <= 2; ++dx) {
                    for (i32 dz = -2; dz <= 2; ++dz) {
                        if (dx * dx + dz * dz <= 4) {
                            i32 world_leaf_x = world_x + dx;
                            i32 world_leaf_z = world_z + dz;
                            glm::ivec3 local_pos = glm::ivec3(
                                (world_leaf_x % Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE) % Chunk::CHUNK_SIZE,
                                y % Chunk::CHUNK_SIZE,
                                (world_leaf_z % Chunk::CHUNK_SIZE + Chunk::CHUNK_SIZE) % Chunk::CHUNK_SIZE
                            );
                            VoxelType leaf_type = VoxelType::LEAVES;

                            // Adjust leaf type based on biome
                            switch (biome) {
                            case BiomeType::SNOWY_MOUNTAINS:
                            case BiomeType::TAIGA:
                                leaf_type = VoxelType::SNOW;
                                break;
                            case BiomeType::BIRCH_FOREST:
                                leaf_type = VoxelType::LEAVES_BIRCH;
                                break;
                            case BiomeType::MANGROVE:
                                leaf_type = VoxelType::MANGROVE_LEAVES;
                                break;
                                // Add more cases as needed
                            default:
                                leaf_type = VoxelType::LEAVES;
                                break;
                            }

                            chunk.SetVoxel(local_pos, leaf_type);
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
                        chunk.SetVoxel(local_pos, voxel_type);
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

    const Sun& Scene::GetSun() const {
        return m_sun;
    }

    void Scene::InsertVoxel(VoxelType voxel_type, const glm::ivec3& world_pos) {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);

        glm::ivec3 chunk_pos, local_pos;
        WorldToChunkLocal(world_pos, chunk_pos, local_pos);

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

        // Insert the voxel
        chunk.SetVoxel(local_pos, voxel_type);
    }

    void Scene::RemoveVoxel(u32 voxel_id) {
        std::lock_guard<std::mutex> lock(m_chunk_mutex);
        auto voxelLocIt = m_voxelLocations.find(voxel_id);
        if (voxelLocIt == m_voxelLocations.end()) {
            LOG_ERROR("Attempted to remove non-existent voxel with ID: " << voxel_id);
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

    VoxelType Scene::GetVoxelAtPosition(const glm::ivec3& world_pos) const {
        glm::ivec3 chunk_pos, local_pos;
        WorldToChunkLocal(world_pos, chunk_pos, local_pos);

        auto chunk_it = m_chunks.find(chunk_pos);
        if (chunk_it != m_chunks.end()) {
            const Chunk& chunk = *chunk_it->second;
            return chunk.GetVoxel(local_pos);
        }

        return VoxelType::AIR;
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
        // Implement raycasting to detect voxel hits
        // Placeholder implementation
        return std::nullopt;
    }

} // namespace MC
