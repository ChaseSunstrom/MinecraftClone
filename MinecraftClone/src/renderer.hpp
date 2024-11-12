#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "scene.hpp"
#include "shader.hpp" 

namespace MC {
    class Renderer {
    public:
        Renderer();
        ~Renderer();

        void Render(const Scene& scene);

    private:
        void InitializeInstanceBuffers(size_t instance_count);

        u64 m_max_instances;
        u32 m_instance_vbo;
        u32 m_color_vbo;
        Shader m_shader;
    };
}
#endif
