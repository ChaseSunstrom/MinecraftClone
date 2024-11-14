#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "types.hpp"
#include "voxel.hpp"
#include "thread_pool.hpp"
#include <array>
#include <mutex>
#include <vector>

namespace MC {
    class Scene;
    class Chunk {
    public:
        static constexpr i32 CHUNK_SIZE = 16;
        static constexpr size_t TOTAL_VOXELS = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

        Chunk(const glm::ivec3& position);

        // Voxel operations
        void SetVoxel(const glm::ivec3& local_pos, VoxelType voxel_type);
        VoxelType GetVoxel(const glm::ivec3& local_pos) const;
        void RemoveVoxel(const glm::ivec3& local_pos);

        // Position of the chunk in chunk coordinates
        glm::ivec3 GetPosition() const;

        // Update the mesh data for rendering
        void UpdateMesh(const Scene& scene);

        // Flag indicating if the chunk needs to update its mesh
        bool NeedsMeshUpdate() const;
        void SetNeedsMeshUpdate(bool needsUpdate);

        bool HasMeshDataGenerated() const;
        bool IsMeshDataUploaded() const;

        void GenerateMeshData(const Scene& scene);
        void UploadMeshData();

        void Update(const Scene& scene, ThreadPool& tp);

        u32 GetVAO() const {
            return m_vao;
        }

        size_t GetIndexCount() const {
            return m_indices.size();
        }

    private:
        inline size_t GetIndex(const glm::ivec3& local_pos) const {
            return local_pos.x + CHUNK_SIZE * (local_pos.y + CHUNK_SIZE * local_pos.z);
        }

    private:
        glm::ivec3 m_position; // Chunk position in chunk coordinates
        std::array<uint8_t, TOTAL_VOXELS> m_voxel_types; // Voxel types in the chunk

        bool m_needs_mesh_update;
        u32 m_vao = 0;
        u32 m_vbo = 0;
        u32 m_ebo = 0;
        std::vector<Vertex> m_vertices;
        std::vector<u32> m_indices;

        std::mutex m_mesh_mutex;
        std::future<void> m_mesh_generation_future;

        // Flags to indicate if mesh data needs uploading
        bool m_mesh_data_generated = false;
        bool m_mesh_data_uploaded = false;
    };
}

#endif // CHUNK_HPP
