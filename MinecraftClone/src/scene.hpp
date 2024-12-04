#ifndef SCENE_HPP
#define SCENE_HPP

#include "chunk.hpp"
#include "camera.hpp"
#include "event_handler.hpp"
#include <FastNoise/FastNoise.h>
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include <glm/glm.hpp>
#include "hash.hpp"
#include "voxel_hit_info.hpp"
#include "thread_pool.hpp"
#include "sun.hpp"
#include <mutex>

namespace MC {
    enum class BiomeType {
        PLAINS,
        DESERT,
        MOUNTAINS,
        FOREST,
        SWAMP,
        JUNGLE,
        SAVANNA,
        TAIGA,
        SNOWY_MOUNTAINS,
        OCEAN,
        TUNDRA,
        BIRCH_FOREST,
        MANGROVE,
        MESA
    };

    class Scene {
    public:
        Scene(EventHandler& event_handler, ThreadPool& tp);
        ~Scene();

        void InitializeScene();

        void SetSkyColor(const glm::vec4& sky_color);

        // Accessors
        Camera& GetCamera() const;
        glm::vec4 GetSkyColor() const;
        const Sun& GetSun() const;

        // Get all chunks
        std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>>& GetChunks();

        // Voxel retrieval
        std::optional<Voxel> GetVoxel(u32 id) const;
        VoxelType GetVoxelAtPosition(const glm::ivec3& world_pos) const;

        // Raycasting
        std::optional<VoxelHitInfo> GetVoxelLookedAt(f32 max_distance = 100.0f) const;

        // Update chunks around the player
        void UpdateChunksAroundPlayer();

        // Insert and remove voxels
        void InsertVoxel(VoxelType voxel_type, const glm::ivec3& world_pos);
        void RemoveVoxel(u32 voxel_id);

        void UpdateChunks();

    private:
        // Helper functions
        void GenerateChunk(const glm::ivec3& chunk_pos);
        void GenerateVoxelDataForChunk(Chunk& chunk);
        void GenerateTrees(Chunk& chunk, i32 world_x, i32 world_z, i32 terrain_height, BiomeType biome);
        BiomeType GetBiomeType(i32 world_x, i32 world_z);
        f32 ComputeElevationNoise(f32 x, f32 z);
        i32 GetBiomeElevation(f32 elevation, BiomeType biome);
        i32 GetTerrainHeight(i32 world_x, i32 world_z, BiomeType biome);
        VoxelType GetVoxelType(i32 world_x, i32 world_y, i32 world_z, i32 terrain_height, BiomeType biome);
        bool IsCave(i32 world_x, i32 world_y, i32 world_z);

    private:
        // Chunks stored by their positions in chunk coordinates
        std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>> m_chunks;

        // Map of voxel IDs to their chunk positions and local positions
        std::unordered_map<u32, std::pair<glm::ivec3, glm::ivec3>> m_voxelLocations;

        std::unique_ptr<Camera> m_camera;
        glm::vec4 m_sky_color;
        glm::ivec3 m_last_player_chunk_pos;
        EventHandler& m_event_handler;
        mutable std::mutex m_chunk_mutex; // Mutex for thread safety
        // Thread pool for chunk generation
        ThreadPool& m_thread_pool;
        u32 m_seed;
        Sun m_sun;

        FastNoise::SmartNode<FastNoise::Perlin> m_elevation_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_elevation_fractal;

        FastNoise::SmartNode<FastNoise::Perlin> m_biome_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_biome_fractal;

        FastNoise::SmartNode<FastNoise::Perlin> m_cave_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_cave_fractal;

        FastNoise::SmartNode<FastNoise::Perlin> m_tree_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_tree_fractal;

        FastNoise::SmartNode<FastNoise::Perlin> m_ore_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_ore_fractal;

        FastNoise::SmartNode<FastNoise::Perlin> m_temperature_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_temperature_fractal;

        FastNoise::SmartNode<FastNoise::Perlin> m_humidity_generator;
        FastNoise::SmartNode<FastNoise::FractalFBm> m_humidity_fractal;

    };
}

#endif // SCENE_HPP
