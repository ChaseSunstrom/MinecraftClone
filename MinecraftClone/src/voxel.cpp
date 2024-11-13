#include "voxel.hpp"

#include <GL/glew.h>

namespace MC {

    std::atomic<u32> Voxel::s_next_id{ 1 }; // Start IDs from 1

    // Static variables for storing the VAO, VBO, and EBO
    u32 Voxel::s_vao = 0;
    u32 Voxel::s_vbo = 0;
    u32 Voxel::s_ebo = 0;

    // Function to map VoxelType to color
    glm::vec4 VoxelTypeToColor(VoxelType type) {
        switch (type) {
        case VoxelType::GRASS:
            return { 0.0f, 1.0f, 0.0f, 1.0f }; // Green
        case VoxelType::DIRT:
            return { 0.55f, 0.27f, 0.07f, 1.0f }; // Brown
        case VoxelType::STONE:
            return { 0.5f, 0.5f, 0.5f, 1.0f }; // Gray
        case VoxelType::SAND:
            return { 0.96f, 0.87f, 0.70f, 1.0f }; // Sand color
        case VoxelType::WOOD:
            return { 0.65f, 0.50f, 0.39f, 1.0f }; // Wood color
        case VoxelType::LEAVES:
            return { 0.13f, 0.55f, 0.13f, 1.0f }; // Dark green
        case VoxelType::AIR:
        default:
            return { 0.0f, 0.0f, 0.0f, 0.0f }; // Transparent
        }
    }

    // Constructors
    Voxel::Voxel()
        : m_id(s_next_id++),
        m_voxel_type(VoxelType::AIR),
        visible_faces(0x3F),
        m_color(VoxelTypeToColor(m_voxel_type)),
        m_local_position(0) {}

    Voxel::Voxel(VoxelType type, const Transform& transform)
        : m_id(s_next_id++),
        m_voxel_type(type),
        m_transform(transform),
        visible_faces(0x3F),
        m_color(VoxelTypeToColor(type)),
        m_local_position(0) {}

    // Face visibility methods
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

    // Accessor methods
    VoxelType Voxel::GetVoxelType() const {
        return m_voxel_type;
    }

    glm::vec4 Voxel::GetColor() const {
        return m_color;
    }

    Transform Voxel::GetTransform() const {
        return m_transform;
    }

    u32 Voxel::GetID() const {
        return m_id;
    }

    glm::vec3 Voxel::GetPos() const {
        return m_transform.GetPos();
    }

    glm::vec3 Voxel::GetRot() const {
        return m_transform.GetRot();
    }

    glm::vec3 Voxel::GetScale() const {
        return m_transform.GetScale();
    }

    glm::ivec3 Voxel::GetLocalPosition() const {
        return m_local_position;
    }

    // Mutator methods
    void Voxel::SetVoxelType(VoxelType type) {
        m_voxel_type = type;
        m_color = VoxelTypeToColor(type);
    }

    void Voxel::SetColor(const glm::vec4& color) {
        m_color = color;
    }

    void Voxel::SetTransform(const Transform& transform) {
        m_transform = transform;
    }

    void Voxel::SetID(u32 id) {
        m_id = id;
    }

    void Voxel::Move(const glm::vec3& pos) {
        m_transform.Move(pos);
    }

    void Voxel::Rotate(const glm::vec3& rot) {
        m_transform.Rotate(rot);
    }

    void Voxel::Scale(const glm::vec3& scale) {
        m_transform.Scale(scale);
    }

    void Voxel::SetLocalPosition(const glm::ivec3& local_pos) {
        m_local_position = local_pos;
    }

    // Static buffer methods
    u32 Voxel::GetVao() {
        return s_vao;
    }

    u32 Voxel::GetVbo() {
        return s_vbo;
    }

    u32 Voxel::GetEbo() {
        return s_ebo;
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
