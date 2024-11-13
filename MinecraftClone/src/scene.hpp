#ifndef SCENE_HPP
#define SCENE_HPP

#include "chunk.hpp"
#include "camera.hpp"
#include "event_handler.hpp"
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>
#include <glm/glm.hpp>
#include "hash.hpp"
#include "voxel_hit_info.hpp"

namespace MC {
    class Scene {
    public:
        Scene(EventHandler& event_handler);
        ~Scene();

        void InsertVoxel(const Voxel& voxel);
        void InsertVoxels(const std::vector<Voxel>& voxels);
        void RemoveVoxel(u32 voxel_id);
        void UpdateVoxel(const Voxel& updated_voxel);
        void InitializeScene();

        void SetSkyColor(const glm::vec4& sky_color);

        // Accessors
        Camera& GetCamera() const;
        glm::vec4 GetSkyColor() const;

        // Get all chunks
        std::unordered_map<glm::ivec3, Chunk>& GetChunks();

        // Voxel retrieval
        std::optional<Voxel> GetVoxel(u32 id) const;
        std::optional<Voxel> GetVoxelAtPosition(const glm::ivec3& world_pos) const;

        // Raycasting
        std::optional<VoxelHitInfo> GetVoxelLookedAt(f32 max_distance = 100.0f) const;

    private:
        // Chunks stored by their positions in chunk coordinates
        std::unordered_map<glm::ivec3, Chunk> m_chunks;

        // Map of voxel IDs to their chunk positions and local positions
        std::unordered_map<u32, std::pair<glm::ivec3, glm::ivec3>> m_voxelLocations;

        std::unique_ptr<Camera> m_camera;
        glm::vec4 m_sky_color;

        EventHandler& m_event_handler;

        mutable std::mutex m_chunk_mutex; // Mutex for thread safety
    };
}

#endif
