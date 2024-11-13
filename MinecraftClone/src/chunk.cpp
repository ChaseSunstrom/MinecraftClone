#include "chunk.hpp"

#include <GL/glew.h>
#include "scene.hpp"

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
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return;
        }

        u32 voxel_id = voxel.GetID();
        m_voxels[voxel_id] = voxel;
        m_voxel_ids[local_pos.z][local_pos.y][local_pos.x] = voxel_id;
        m_needs_mesh_update = true;

        Voxel voxel_copy = voxel;
        voxel_copy.SetLocalPosition(local_pos);
        m_voxels[voxel_id] = voxel_copy;
    }


    std::optional<Voxel> Chunk::GetVoxel(const glm::ivec3& local_pos) const {
        if (local_pos.x < 0 || local_pos.x >= CHUNK_SIZE ||
            local_pos.y < 0 || local_pos.y >= CHUNK_SIZE ||
            local_pos.z < 0 || local_pos.z >= CHUNK_SIZE) {
            return std::nullopt;
        }

        auto voxel_id_opt = m_voxel_ids[local_pos.z][local_pos.y][local_pos.x];
        if (voxel_id_opt.has_value()) {
            u32 voxel_id = voxel_id_opt.value();
            auto it = m_voxels.find(voxel_id);
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

    bool Chunk::HasMeshDataGenerated() const {
        return m_mesh_data_generated;
    }

    bool Chunk::IsMeshDataUploaded() const {
        return m_mesh_data_uploaded;
    }

    void Chunk::SetNeedsMeshUpdate(bool needsUpdate) {
        m_needs_mesh_update = needsUpdate;
    }

    void Chunk::GenerateMeshData(const Scene& scene) {
        std::lock_guard<std::mutex> lock(m_mesh_mutex);

        m_vertices.clear();
        m_indices.clear();
        GLuint index_offset = 0;

        static const glm::ivec3 directions[6] = {
            {1, 0, 0},  // POS_X
            {-1, 0, 0}, // NEG_X
            {0, 1, 0},  // POS_Y
            {0, -1, 0}, // NEG_Y
            {0, 0, 1},  // POS_Z
            {0, 0, -1}  // NEG_Z
        };

        static const glm::vec3 faceNormals[6] = {
            {1, 0, 0},   // POS_X
            {-1, 0, 0},  // NEG_X
            {0, 1, 0},   // POS_Y
            {0, -1, 0},  // NEG_Y
            {0, 0, 1},   // POS_Z
            {0, 0, -1}   // NEG_Z
        };

        for (auto& [voxel_id, voxel] : m_voxels) {
            glm::ivec3 local_pos = voxel.GetLocalPosition();
            glm::vec3 worldPos = glm::vec3(local_pos);

            // For each face
            for (int i = 0; i < 6; ++i) {
                glm::ivec3 neighbor_pos = local_pos + directions[i];

                // Check if neighbor voxel exists
                std::optional<Voxel> neighbor_voxel;
                if (neighbor_pos.x >= 0 && neighbor_pos.x < CHUNK_SIZE &&
                    neighbor_pos.y >= 0 && neighbor_pos.y < CHUNK_SIZE &&
                    neighbor_pos.z >= 0 && neighbor_pos.z < CHUNK_SIZE) {
                    neighbor_voxel = GetVoxel(neighbor_pos);
                }
                else {
                    // Neighbor might be in adjacent chunk
                    glm::ivec3 neighbor_world_pos = glm::ivec3(worldPos + glm::vec3(m_position * CHUNK_SIZE) + glm::vec3(directions[i]));
                    neighbor_voxel = scene.GetVoxelAtPosition(neighbor_world_pos);
                }

                if (!neighbor_voxel.has_value()) {
                    // Neighbor voxel does not exist, so this face is visible
                    // Create face vertices and indices
                    Vertex face_vertices[4];
                    for (i32 j = 0; j < 4; ++j) {
                        face_vertices[j].pos = VOXEL_FACE_VERTICES[i][j] + worldPos;
                        face_vertices[j].normal = faceNormals[i];
                        face_vertices[j].color = voxel.GetColor();
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(GLuint), m_indices.data(), GL_STATIC_DRAW);

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
            mesh_generation_future = tp.Enqueue(TaskPriority::VERY_HIGH, true, [this, &scene]() {
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
