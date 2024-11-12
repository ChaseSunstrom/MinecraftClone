#ifndef SCENE_HPP
#define SCENE_HPP

#include "voxel.hpp"
#include "camera.hpp"
#include <unordered_map>
#include <memory>
#include <optional>
#include <vector>

namespace MC {
    class Scene {
    public:
        static Scene& GetScene();
        void InsertVoxel(const Voxel& voxel);
        void RemoveVoxel(u32 voxel_id);
        void UpdateVoxel(const Voxel& updated_voxel);
        void InitializeScene();

        void SetSkyColor(const glm::vec4& sky_color);

        const std::unordered_map<VoxelColor, std::vector<Transform>>& GetVoxelTransforms() const;
        const std::unordered_map<VoxelColor, std::vector<glm::mat4>>& GetVoxelMatrices() const;
        const std::unordered_map<u32, Voxel>& GetVoxels() const;
        std::optional<Voxel> GetVoxel(u64 id) const;

        Camera& GetCamera() const;
        glm::vec4 GetSkyColor() const;

    private:
        Scene();
        ~Scene();

    private:
        std::unordered_map<VoxelColor, std::vector<Transform>> m_voxel_transforms;
        std::unordered_map<VoxelColor, std::vector<glm::mat4>> m_voxel_matrices;
        std::unordered_map<u32, Voxel> m_voxels;

        std::unordered_map<u32, size_t> m_id_to_index; // Map voxel ID to its index in m_voxels

        std::unique_ptr<Camera> m_camera;
        glm::vec4 m_sky_color;
    };
}

#endif
