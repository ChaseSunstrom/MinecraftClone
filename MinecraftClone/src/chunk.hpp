#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "voxel.hpp"
#include "types.hpp"
#include "hash.hpp"
#include <unordered_map>
#include <glm/glm.hpp>
#include <array>
#include <optional>

namespace MC {
    class Chunk {
    public:
        static const i32 CHUNK_SIZE = 16;

        Chunk(const glm::ivec3& position);

        // Voxel operations
        void SetVoxel(const glm::ivec3& local_pos, const Voxel& voxel);
        std::optional<Voxel> GetVoxel(const glm::ivec3& local_pos) const;
        void RemoveVoxel(const glm::ivec3& local_pos);

        // Position of the chunk in chunk coordinates
        glm::ivec3 GetPosition() const;

        // Accessors for rendering
        const std::unordered_map<u32, Voxel>& GetVoxels() const;

        // Update the mesh data for rendering
        void UpdateMesh();

        // Flag indicating if the chunk needs to update its mesh
        bool NeedsMeshUpdate() const;
        void SetNeedsMeshUpdate(bool needsUpdate);

    private:
        glm::ivec3 m_position; // Chunk position in chunk coordinates
        std::unordered_map<u32, Voxel> m_voxels; // Voxels in the chunk
        std::array<std::array<std::array<std::optional<u32>, CHUNK_SIZE>, CHUNK_SIZE>, CHUNK_SIZE> m_voxel_ids;
        bool m_needs_mesh_update;

    };
}

#endif // CHUNK_HPP
