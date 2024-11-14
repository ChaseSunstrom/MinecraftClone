#ifndef SUN_HPP
#define SUN_HPP

#include "types.hpp"
#include "vertex.hpp"

namespace MC {

	class Sun {
	public:
        Sun() = default;
        ~Sun();

        void Initialize();

        u32 GetVAO() const { return m_sun_vao; }
        u32 GetVBO() const { return m_sun_vbo; }
        u32 GetEBO() const { return m_sun_ebo; }
	private:
        u32 m_sun_vao = 0;
        u32 m_sun_vbo = 0;
        u32 m_sun_ebo = 0;
	};

    const Vertex SUN_VERTICES[8] = {
        // Define the 8 corners of the cube with positions, normals, and bright colors
        // Front Face
        { {-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 0
        { { 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 1
        { { 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 2
        { {-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 3

        // Back Face
        { {-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f, 1.0f }, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 4
        { { 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f, 1.0f }, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 5
        { { 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f, 1.0f }, {1.0f, 0.95f, 0.8f, 1.0f} }, // Vertex 6
        { {-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f, 1.0f }, {1.0f, 0.95f, 0.8f, 1.0f} }  // Vertex 7
    };


    const u32 SUN_INDICES[36] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        0, 3, 7, 7, 4, 0,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Top face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        0, 1, 5, 5, 4, 0
    };
}


#endif