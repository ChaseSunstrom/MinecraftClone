#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "scene.hpp"
#include "shader.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <array>

namespace MC {
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Render(Scene& scene);

    private:
        Shader m_shader;
        u32 m_instance_vbo;

        // Chunk VAO and VBOs
        u32 m_chunk_vao;
        u32 m_chunk_vbo;
        u32 m_chunk_ebo;

        void InitializeBuffers();
    };
}

#endif // RENDERER_HPP
