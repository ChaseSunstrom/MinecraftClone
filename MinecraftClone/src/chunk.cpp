#include "chunk.hpp"

namespace MC {
    Chunk::Chunk(const glm::ivec3& position)
        : m_position(position), m_needs_mesh_update(true) {
        // Initialize voxel ID array to null
        for (auto& plane : m_voxel_ids)
            for (auto& row : plane)
                for (auto& voxel_id : row)
                    voxel_id = std::nullopt;
    }

    void Chunk::SetVoxel(const glm::ivec3& local_pos, const Voxel& voxel) {
        // Ensure local_pos is within chunk bounds
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return;
        }

        u32 voxel_id = voxel.GetID();
        m_voxels[voxel_id] = voxel;
        m_voxel_ids[local_pos.x][local_pos.y][local_pos.z] = voxel_id;
        m_needs_mesh_update = true;
    }

    std::optional<Voxel> Chunk::GetVoxel(const glm::ivec3& local_pos) const {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return std::nullopt;
        }

        auto voxelIdOpt = m_voxel_ids[local_pos.x][local_pos.y][local_pos.z];
        if (voxelIdOpt.has_value()) {
            u32 voxelId = voxelIdOpt.value();
            auto it = m_voxels.find(voxelId);
            if (it != m_voxels.end()) {
                return it->second;
            }
        }
        return std::nullopt;
    }

    void Chunk::RemoveVoxel(const glm::ivec3& local_pos) {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return;
        }

        auto voxel_id_opt = m_voxel_ids[local_pos.x][local_pos.y][local_pos.z];
        if (voxel_id_opt.has_value()) {
            u32 voxel_id = voxel_id_opt.value();
            m_voxels.erase(voxel_id);
            m_voxel_ids[local_pos.x][local_pos.y][local_pos.z] = std::nullopt;
            m_needs_mesh_update = true;
        }
    }

    glm::ivec3 Chunk::GetPosition() const {
        return m_position;
    }

    const std::unordered_map<u32, Voxel>& Chunk::GetVoxels() const {
        return m_voxels;
    }

    bool Chunk::NeedsMeshUpdate() const {
        return m_needs_mesh_update;
    }

    void Chunk::SetNeedsMeshUpdate(bool needsUpdate) {
        m_needs_mesh_update = needsUpdate;
    }

    void Chunk::UpdateMesh() {
        // Implement mesh update logic here
        // This will generate the mesh data for rendering
        m_needs_mesh_update = false;
    }
}
