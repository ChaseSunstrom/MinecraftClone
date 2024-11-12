// Scene.hpp
#ifndef SCENE_HPP
#define SCENE_HPP

#include "voxel.hpp"
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
        void RemoveVoxel(u32 voxel_id);
        void UpdateVoxel(const Voxel& updated_voxel);
        void InitializeScene();

        void SetSkyColor(const glm::vec4& sky_color);

        const std::unordered_map<VoxelColor, std::vector<Transform>>& GetVoxelTransforms() const;
        const std::unordered_map<VoxelColor, std::vector<glm::mat4>>& GetVoxelMatrices() const;
        const std::unordered_map<u32, Voxel>& GetVoxels() const;
        std::optional<Voxel> GetVoxel(u32 id) const;

        Camera& GetCamera() const;
        glm::vec4 GetSkyColor() const;

        std::optional<Voxel> GetVoxelAtPosition(const glm::ivec3& grid_pos) const;
        std::optional<VoxelHitInfo> GetVoxelLookedAt(f32 max_distance = 100.0f) const;

    private:
        std::unordered_map<VoxelColor, std::vector<Transform>> m_voxel_transforms;
        std::unordered_map<VoxelColor, std::vector<glm::mat4>> m_voxel_matrices;
        std::unordered_map<u32, Voxel> m_voxels;

        std::unordered_map<glm::ivec3, u32> m_position_to_id;

        std::unique_ptr<Camera> m_camera;
        glm::vec4 m_sky_color;

        EventHandler& m_event_handler;

        mutable std::mutex m_voxel_mutex; // Mutex for thread safety
    };
}

#endif
