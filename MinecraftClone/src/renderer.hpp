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
        ~Renderer() = default;

        void Render(ThreadPool& tp, Scene& scene);
        void RenderSun(const Sun& sun, const Camera& camera, const glm::vec3& light_direction);

        void EnableLighting(bool enable);
        bool IsLightingEnabled() const;
    public:
    private:
        // Normally I would not hardcode these but this is just a simple minecraft clone, nothing fancy
        Shader m_lit_shader;
        Shader m_unlit_shader;
        bool m_enable_lighting;
    };
}

#endif // RENDERER_HPP
