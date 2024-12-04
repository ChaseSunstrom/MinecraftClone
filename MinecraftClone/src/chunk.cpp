#include "chunk.hpp"

#include <GL/glew.h>
#include "scene.hpp"

namespace MC {
    Chunk::Chunk(const glm::ivec3& position)
        : m_position(position), m_needs_mesh_update(true) {
        // Initialize all voxels to AIR
        m_voxel_types.fill(static_cast<u32>(VoxelType::AIR));
    }

    void Chunk::SetVoxel(const glm::ivec3& local_pos, VoxelType voxel_type) {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return;
        }

        size_t index = GetIndex(local_pos);
        m_voxel_types[index] = static_cast<u32>(voxel_type);
        m_needs_mesh_update = true;
    }

    VoxelType Chunk::GetVoxel(const glm::ivec3& local_pos) const {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return VoxelType::AIR;
        }

        size_t index = GetIndex(local_pos);
        return static_cast<VoxelType>(m_voxel_types[index]);
    }

    void Chunk::RemoveVoxel(const glm::ivec3& local_pos) {
        SetVoxel(local_pos, VoxelType::AIR);
    }

    glm::ivec3 Chunk::GetPosition() const {
        return m_position;
    }

    bool Chunk::NeedsMeshUpdate() const {
        return m_needs_mesh_update;
    }

    bool Chunk::HasMeshDataGenerated() const {
        return m_mesh_data_generated;
    }

    bool Chunk::IsMeshDataUploaded() const {
        return m_mesh_data_uploaded;
    }

    void Chunk::SetNeedsMeshUpdate(bool needs_update) {
        m_needs_mesh_update = needs_update;
    }

    void Chunk::GenerateMeshData(const Scene& scene) {
        std::lock_guard<std::mutex> lock(m_mesh_mutex);

        m_vertices.clear();
        m_indices.clear();
        u32 index_offset = 0;

        static const glm::ivec3 directions[6] = {
            {1, 0, 0},   // POS_X
            {-1, 0, 0},  // NEG_X
            {0, 1, 0},   // POS_Y
            {0, -1, 0},  // NEG_Y
            {0, 0, 1},   // POS_Z
            {0, 0, -1}   // NEG_Z
        };

        static const glm::vec3 face_normals[6] = {
            {1.0f, 0.0f, 0.0f},   // POS_X
            {-1.0f, 0.0f, 0.0f},  // NEG_X
            {0.0f, 1.0f, 0.0f},   // POS_Y
            {0.0f, -1.0f, 0.0f},  // NEG_Y
            {0.0f, 0.0f, 1.0f},   // POS_Z
            {0.0f, 0.0f, -1.0f}   // NEG_Z
        };

        // Loop over all positions in the chunk
        for (i32 x = 0; x < CHUNK_SIZE; ++x) {
            for (i32 y = 0; y < CHUNK_SIZE; ++y) {
                for (i32 z = 0; z < CHUNK_SIZE; ++z) {
                    glm::ivec3 local_pos(x, y, z);
                    size_t index = GetIndex(local_pos);
                    VoxelType voxel_type = static_cast<VoxelType>(m_voxel_types[index]);

                    // Skip empty voxels
                    if (voxel_type == VoxelType::AIR) {
                        continue;
                    }

                    glm::vec3 world_pos = glm::vec3(x, y, z);

                    // For each face
                    for (i32 i = 0; i < 6; ++i) {
                        glm::ivec3 neighbor_pos = local_pos + directions[i];
                        VoxelType neighbor_voxel_type;

                        if (neighbor_pos.x >= 0 && neighbor_pos.x < CHUNK_SIZE &&
                            neighbor_pos.y >= 0 && neighbor_pos.y < CHUNK_SIZE &&
                            neighbor_pos.z >= 0 && neighbor_pos.z < CHUNK_SIZE) {
                            // Neighbor is within this chunk
                            size_t neighbor_index = GetIndex(neighbor_pos);
                            neighbor_voxel_type = static_cast<VoxelType>(m_voxel_types[neighbor_index]);
                        }
                        else {
                            // Neighbor is in a different chunk
                            glm::ivec3 neighbor_world_pos = m_position * CHUNK_SIZE + neighbor_pos;
                            neighbor_voxel_type = scene.GetVoxelAtPosition(neighbor_world_pos);
                        }

                        // If the neighbor voxel is AIR or transparent, render this face
                        if (neighbor_voxel_type == VoxelType::AIR) {
                            // Create face vertices and indices
                            Vertex face_vertices[4];
                            for (i32 j = 0; j < 4; ++j) {
                                face_vertices[j].pos = VOXEL_FACE_VERTICES[i][j] + world_pos;
                                face_vertices[j].normal = face_normals[i];
                                face_vertices[j].color = VoxelTypeToColor(voxel_type);
                            }

                            m_vertices.insert(m_vertices.end(), face_vertices, face_vertices + 4);

                            m_indices.push_back(index_offset + 0);
                            m_indices.push_back(index_offset + 1);
                            m_indices.push_back(index_offset + 2);
                            m_indices.push_back(index_offset + 2);
                            m_indices.push_back(index_offset + 3);
                            m_indices.push_back(index_offset + 0);

                            index_offset += 4;
                        }
                    }
                }
            }
        }

        m_mesh_data_generated = true;
        m_mesh_data_uploaded = false;
    }


    void Chunk::UploadMeshData() {
        std::lock_guard<std::mutex> lock(m_mesh_mutex);

        if (!m_mesh_data_generated || m_mesh_data_uploaded) {
            return; // Nothing to upload
        }

        // Generate or update VBOs
        if (m_vao == 0) {
            glGenVertexArrays(1, &m_vao);
            glGenBuffers(1, &m_vbo);
            glGenBuffers(1, &m_ebo);
        }

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(u32), m_indices.data(), GL_STATIC_DRAW);

        // Vertex attributes
        glEnableVertexAttribArray(0); // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

        glEnableVertexAttribArray(1); // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2); // Color attribute
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

        glBindVertexArray(0);

        m_needs_mesh_update = false;
        m_mesh_data_uploaded = true;
    }

    void Chunk::Update(const Scene& scene, ThreadPool& tp) {
        if (NeedsMeshUpdate() && !HasMeshDataGenerated()) {
            // Enqueue mesh generation
            m_mesh_generation_future = tp.Enqueue(TaskPriority::VERY_HIGH, true, [this, &scene]() {
                    GenerateMeshData(scene);
                    SetNeedsMeshUpdate(false);
                });
        }
        else if (HasMeshDataGenerated() && !IsMeshDataUploaded()) {
            // Upload mesh data on the main thread
            UploadMeshData();
        }
    }

}
