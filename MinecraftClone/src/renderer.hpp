#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "scene.hpp"
#include "shader.hpp" 

#include <array>

namespace MC {
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Render(const Scene& scene);

    private:
        void InitializeInstanceBuffers(size_t instance_count);
        void InitializeFaceBuffers();
        void UpdateInstanceData(const Scene& scene);
    private:
        u64 m_max_instances;
        u32 m_instance_vbo;
        Shader m_shader;
        std::array<u32, 6> m_face_vaos; // VAOs for each face direction
        std::array<u32, 6> m_face_vbos; // VBOs for each face direction
        std::array<u32, 6> m_face_ebos; // VBOs for each face direction
        std::unordered_map<i32, std::unordered_map<VoxelColor, std::vector<glm::mat4>>> m_face_instance_matrices;
    };
}
#endif
