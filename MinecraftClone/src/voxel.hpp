#ifndef VOXEL_HPP
#define VOXEL_HPP

#include "transform.hpp"
#include "types.hpp"
#include "vertex.hpp"

#include <array>
#include <atomic>

namespace MC {
	enum class VoxelType {
		AIR,
		BEDROCK,
		WATER,
		SAND,
		GRASS_PLAINS,
		GRASS_FOREST,
		GRASS_JUNGLE,
		GRASS_SAVANNA,
		GRASS_TAIGA,
		GRASS_BIRCH,
		MANGROVE_WOOD,
		RED_SAND,
		DIRT,
		STONE,
		SNOW,
		WOOD,
		LEAVES,
		LEAVES_BIRCH,
		MANGROVE_LEAVES,
		DIAMOND_ORE,
		GOLD_ORE,
		IRON_ORE,
		COAL_ORE,
		ICE,
		GRAVEL,
		LAVA
	};

	// Function to map VoxelType to color
	glm::vec4 VoxelTypeToColor(VoxelType type);

	class Voxel {
	public:
		enum FaceIndex {
			POS_X = 0,
			NEG_X,
			POS_Y,
			NEG_Y,
			POS_Z,
			NEG_Z
		};

		Voxel();
		Voxel(VoxelType type);

		bool IsFaceVisible(FaceIndex face) const;
		void SetFaceVisible(FaceIndex face, bool visible);

		VoxelType GetVoxelType() const;
		glm::vec4 GetColor() const;
		u32 GetID() const;
		glm::ivec3 GetLocalPosition() const;

		void SetVoxelType(VoxelType type);
		void SetID(u32 id);
		void SetLocalPosition(const glm::ivec3& local_pos);

		// Static methods for buffer management
		static void InitializeStaticBuffers();
		static void CleanupStaticBuffers();
		static u32 GetVao();
		static u32 GetVbo();
		static u32 GetEbo();

	public:
		uint8_t visible_faces;

	private:
		VoxelType m_voxel_type;
		glm::ivec3 m_local_position;
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

	constexpr glm::vec3 VOXEL_FACE_VERTICES[6][4] = {
		// POS_X Face
		{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}},
		// NEG_X Face
		{{0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0}},
		// POS_Y Face
		{{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
		// NEG_Y Face
		{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
		// POS_Z Face
		{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
		// NEG_Z Face
		{{0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0}}
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