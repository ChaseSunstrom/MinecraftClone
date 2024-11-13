#ifndef VOXEL_HPP
#define VOXEL_HPP

#include "transform.hpp"
#include "types.hpp"
#include "vertex.hpp"

#include <array>
#include <atomic>

namespace MC {
	struct NeighboringFaces {
		bool front : 1;
		bool back : 1;
		bool top : 1;
		bool bottom : 1;
		bool left : 1;
		bool right : 1;
		bool align2 : 2;
	};

	enum class VoxelColor {
		RED,
		GREEN,
		BLUE,
		BLACK,
		PINK,
		WHITE,
		PURPLE,
		ORANGE,
		YELLOW,
		BROWN,
		CYAN,
		MAGENTA,
		GRAY,
		LIGHT_BLUE,
		LIGHT_GREEN,
		DARK_RED
	};
	glm::vec4 VoxelColorToColor(VoxelColor color);

	// Since we are only using Voxels (for now atleast) we will just store the VAO, VBO, EBO, statically
	class Voxel {
	public:
		Voxel(VoxelColor color, const Transform& transform = Transform(), NeighboringFaces neighboring_faces = {})
		: m_id(s_next_id++), // Assign unique ID
		m_voxel_color(color),
		m_transform(transform),
		m_neighboring_faces(neighboring_faces),
		m_color(VoxelColorToColor(color)) { }

		Voxel& operator=(const Voxel& voxel) {
			m_id = s_next_id++;
			m_voxel_color = voxel.m_voxel_color;
			m_transform = voxel.m_transform;
			m_neighboring_faces = voxel.m_neighboring_faces;
			m_color = voxel.m_color;
			return *this;
		}
		
		VoxelColor GetVoxelColor() const;
		glm::vec4 GetColor() const;
		Transform GetTransform() const;
		NeighboringFaces GetNeighboringFaces() const;

		u32 GetID() const { return m_id; }
		void SetID(u32 id) { m_id = id; }

		void SetVoxelColor(VoxelColor color);
		void SetColor(const glm::vec4& color);
		void SetTransform(const Transform& transform);
		void SetNeighboringFaces(NeighboringFaces neighboring_faces);

		void Move(const glm::vec3& pos);
		void Rotate(const glm::vec3& rot);
		void Scale(const glm::vec3& scale);

		glm::vec3 GetPos() const;
		glm::vec3 GetRot() const;
		glm::vec3 GetScale() const;

		static void InitializeStaticBuffers();
		static void CleanupStaticBuffers();
		static u32 GetVao();
		static u32 GetVbo();
		static u32 GetEbo();
	private:
		Transform m_transform;
		NeighboringFaces m_neighboring_faces;
		// Color for now, textures will be implemented later
		VoxelColor m_voxel_color;
		glm::vec4 m_color;
		static u32 s_vao;
		static u32 s_vbo;
		static u32 s_ebo;


		static std::atomic<u32> s_next_id; // Atomic for thread safety
		u32 m_id;
	};

	constexpr Vertex VOXEL_VERTICES[] = {
		// Front face
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 0
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 1
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 2
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 3

		// Back face
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}}, // 4
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}}, // 5
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}}, // 6
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}}, // 7

		// Left face
		{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}}, // 8
		{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}}, // 9
		{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}}, // 10
		{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}}, // 11

		// Right face
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 12
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 13
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 14
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // 15

		// Top face
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // 16
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // 17
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 18
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 19

		// Bottom face
		{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}}, // 20
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}}, // 21
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}}, // 22
		{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}}, // 23
	};

	constexpr u16 VOXEL_INDICES[] = {
		// Front face
		0, 1, 2, 2, 3, 0,
		// Back face
		4, 5, 6, 6, 7, 4,
		// Left face
		8, 9, 10, 10, 11, 8,
		// Right face
		12, 13, 14, 14, 15, 12,
		// Top face
		16, 17, 18, 18, 19, 16,
		// Bottom face
		20, 21, 22, 22, 23, 20
	};
}


#endif