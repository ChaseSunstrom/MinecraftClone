#include "voxel.hpp"

#include <GL/glew.h>

namespace MC {
	std::atomic<u32> Voxel::s_next_id{ 1 }; // Start IDs from 1

	// Static variables for storing the VAO, VBO, and EBO
	u32 Voxel::s_vao = 0;
	u32 Voxel::s_vbo = 0;
	u32 Voxel::s_ebo = 0;

	u32 Voxel::GetVao() {
		return s_vao;
	}

	u32 Voxel::GetVbo() {
		return s_vbo;
	}

	u32 Voxel::GetEbo() {
		return s_ebo;
	}

	void Voxel::InitializeStaticBuffers()
	{
		if (s_vao == 0)
		{
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

	void Voxel::CleanupStaticBuffers()
	{
		// Delete buffers only if they exist
		if (s_ebo != 0)
		{
			glDeleteBuffers(1, &s_ebo);
			s_ebo = 0;
		}
		if (s_vbo != 0)
		{
			glDeleteBuffers(1, &s_vbo);
			s_vbo = 0;
		}
		if (s_vao != 0)
		{
			glDeleteVertexArrays(1, &s_vao);
			s_vao = 0;
		}
	}

	glm::vec4 VoxelColorToColor(VoxelColor color) {
		switch (color) {
		case VoxelColor::RED:
			return { 1.0, 0.0, 0.0, 1.0 };
		case VoxelColor::GREEN:
			return { 0.0, 1.0, 0.0, 1.0 };
		case VoxelColor::BLUE:
			return { 0.0, 0.0, 1.0, 1.0 };
		case VoxelColor::BLACK:
			return { 0.0, 0.0, 0.0, 1.0 };
		case VoxelColor::PINK:
			return { 1.0, 0.75, 0.8, 1.0 };
		case VoxelColor::WHITE:
			return { 1.0, 1.0, 1.0, 1.0 };
		case VoxelColor::PURPLE:
			return { 0.5, 0.0, 0.5, 1.0 };
		case VoxelColor::ORANGE:
			return { 1.0, 0.65, 0.0, 1.0 };
		case VoxelColor::YELLOW:
			return { 1.0, 1.0, 0.0, 1.0 };
		case VoxelColor::BROWN:
			return { 0.65, 0.16, 0.16, 1.0 };
		case VoxelColor::CYAN:
			return { 0.0, 1.0, 1.0, 1.0 };
		case VoxelColor::MAGENTA:
			return { 1.0, 0.0, 1.0, 1.0 };
		case VoxelColor::GRAY:
			return { 0.5, 0.5, 0.5, 1.0 };
		case VoxelColor::LIGHT_BLUE:
			return { 0.68, 0.85, 0.9, 1.0 };
		case VoxelColor::LIGHT_GREEN:
			return { 0.56, 0.93, 0.56, 1.0 };
		case VoxelColor::DARK_RED:
			return { 0.55, 0.0, 0.0, 1.0 };
		default:
			return { 0.0, 0.0, 0.0, 1.0 }; // Default to black if color not recognized
		}
	}

	VoxelColor Voxel::GetVoxelColor() const {
		return m_voxel_color;
	}

	glm::vec4 Voxel::GetColor() const {
		return m_color;
	}
	
	Transform Voxel::GetTransform() const {
		return m_transform;
	}
	
	NeighboringFaces Voxel::GetNeighboringFaces() const {
		return m_neighboring_faces;
	}

	void Voxel::SetVoxelColor(VoxelColor color) {
		m_voxel_color = color;
		m_color = VoxelColorToColor(color);
	}
	
	void Voxel::SetColor(const glm::vec4& color) {
		m_color = color;
	}
	
	void Voxel::SetTransform(const Transform& transform) {
		m_transform = transform;
	}
	
	void Voxel::SetNeighboringFaces(NeighboringFaces neighboring_faces) {
		m_neighboring_faces = neighboring_faces;
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

	glm::vec3 Voxel::GetPos() const {
		return m_transform.GetPos();
	}

	glm::vec3 Voxel::GetRot() const {
		return m_transform.GetRot();
	}

	glm::vec3 Voxel::GetScale() const {
		return m_transform.GetScale();
	}
}