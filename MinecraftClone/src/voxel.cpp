#include "voxel.hpp"

#include <GL/glew.h>

namespace MC {

    std::atomic<u32> Voxel::s_next_id{ 1 }; // Start IDs from 1

    // Static variables for storing the VAO, VBO, and EBO
    u32 Voxel::s_vao = 0;
    u32 Voxel::s_vbo = 0;
    u32 Voxel::s_ebo = 0;

    glm::vec4 VoxelTypeToColor(VoxelType type) {
        switch (type) {
        case VoxelType::GRASS_PLAINS:
            return { 0.0f, 0.8f, 0.0f, 1.0f }; // Bright green
        case VoxelType::GRASS_FOREST:
            return { 0.0f, 0.6f, 0.0f, 1.0f }; // Darker green
        case VoxelType::GRASS_JUNGLE:
            return { 0.0f, 0.9f, 0.2f, 1.0f }; // Lush green
        case VoxelType::GRASS_SAVANNA:
            return { 0.5f, 0.8f, 0.0f, 1.0f }; // Yellowish green
        case VoxelType::GRASS_TAIGA:
            return { 0.0f, 0.7f, 0.5f, 1.0f }; // Bluish green
        case VoxelType::GRASS_BIRCH:
            return { 0.6f, 0.8f, 0.6f, 1.0f }; // Light green
        case VoxelType::MANGROVE_WOOD:
            return { 0.55f, 0.27f, 0.07f, 1.0f }; // Brown
        case VoxelType::RED_SAND:
            return { 0.8f, 0.4f, 0.2f, 1.0f }; // Red sand
        case VoxelType::DIRT:
            return { 0.55f, 0.27f, 0.07f, 1.0f }; // Brown
        case VoxelType::STONE:
            return { 0.5f, 0.5f, 0.5f, 1.0f }; // Gray
        case VoxelType::SNOW:
            return { 1.0f, 1.0f, 1.0f, 1.0f }; // White
        case VoxelType::WOOD:
            return { 0.65f, 0.50f, 0.39f, 1.0f }; // Wood color
        case VoxelType::LEAVES:
            return { 0.13f, 0.55f, 0.13f, 1.0f }; // Dark green
        case VoxelType::LEAVES_BIRCH:
            return { 0.8f, 0.9f, 0.6f, 1.0f }; // Light yellowish green
        case VoxelType::MANGROVE_LEAVES:
            return { 0.0f, 0.5f, 0.0f, 1.0f }; // Dark green
        case VoxelType::DIAMOND_ORE:
            return { 0.0f, 1.0f, 1.0f, 1.0f }; // Cyan
        case VoxelType::GOLD_ORE:
            return { 1.0f, 0.84f, 0.0f, 1.0f }; // Gold color
        case VoxelType::IRON_ORE:
            return { 0.8f, 0.5f, 0.2f, 1.0f }; // Rusty color
        case VoxelType::COAL_ORE:
            return { 0.2f, 0.2f, 0.2f, 1.0f }; // Dark gray
        case VoxelType::WATER:
            return { 0.0f, 0.0f, 1.0f, 0.7f }; // Blue with transparency
        case VoxelType::ICE:
            return { 0.7f, 0.9f, 1.0f, 0.8f }; // Light blue with transparency
        case VoxelType::GRAVEL:
            return { 0.6f, 0.6f, 0.6f, 1.0f }; // Light gray
        case VoxelType::LAVA:
            return { 1.0f, 0.5f, 0.0f, 1.0f }; // Orange
        case VoxelType::BEDROCK:
            return { 0.1f, 0.1f, 0.1f, 1.0f }; // Almost black
        case VoxelType::AIR:
        default:
            return { 0.0f, 0.0f, 0.0f, 0.0f }; // Transparent
        }
    }

    Voxel::Voxel()
        : m_id(s_next_id++),
        m_voxel_type(VoxelType::AIR),
        visible_faces(0x3F),
        m_local_position(0) {}

    Voxel::Voxel(VoxelType type)
        : m_id(s_next_id++),
        m_voxel_type(type),
        visible_faces(0x3F),
        m_local_position(0) {}

    bool Voxel::IsFaceVisible(FaceIndex face) const {
        return (visible_faces & (1 << face)) != 0;
    }

    void Voxel::SetFaceVisible(FaceIndex face, bool visible) {
        if (visible) {
            visible_faces |= (1 << face);
        }
        else {
            visible_faces &= ~(1 << face);
        }
    }

    VoxelType Voxel::GetVoxelType() const {
        return m_voxel_type;
    }

    glm::vec4 Voxel::GetColor() const {
        return VoxelTypeToColor(m_voxel_type);
    }

    u32 Voxel::GetID() const {
        return m_id;
    }

    glm::ivec3 Voxel::GetLocalPosition() const {
        return m_local_position;
    }

    void Voxel::SetVoxelType(VoxelType type) {
        m_voxel_type = type;
    }

    void Voxel::SetID(u32 id) {
        m_id = id;
    }

    void Voxel::SetLocalPosition(const glm::ivec3& local_pos) {
        m_local_position = local_pos;
    }

    void Voxel::InitializeStaticBuffers() {
        if (s_vao == 0) {
            // Generate and bind the VAO
            glGenVertexArrays(1, &s_vao);
            glBindVertexArray(s_vao);

            // Generate and bind the VBO
            glGenBuffers(1, &s_vbo);
            glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(VOXEL_VERTICES), VOXEL_VERTICES, GL_STATIC_DRAW);

            // Generate and bind the EBO
            glGenBuffers(1, &s_ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VOXEL_INDICES), VOXEL_INDICES, GL_STATIC_DRAW);

            // Define the vertex attributes (position and normal)
            constexpr i32 POSITION_ATTRIB_INDEX = 0;
            constexpr i32 NORMAL_ATTRIB_INDEX = 1;

            // Position attribute
            glVertexAttribPointer(POSITION_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(POSITION_ATTRIB_INDEX);

            // Normal attribute
            glVertexAttribPointer(NORMAL_ATTRIB_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
            glEnableVertexAttribArray(NORMAL_ATTRIB_INDEX);

            // Unbind the VAO (optional for safety)
            glBindVertexArray(0);
        }
    }

    void Voxel::CleanupStaticBuffers() {
        // Delete buffers only if they exist
        if (s_ebo != 0) {
            glDeleteBuffers(1, &s_ebo);
            s_ebo = 0;
        }
        if (s_vbo != 0) {
            glDeleteBuffers(1, &s_vbo);
            s_vbo = 0;
        }
        if (s_vao != 0) {
            glDeleteVertexArrays(1, &s_vao);
            s_vao = 0;
        }
    }

} // namespace MC
