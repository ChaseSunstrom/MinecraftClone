#include "renderer.hpp"
#include "voxel.hpp"
#include <GL/glew.h>

namespace MC {
    Renderer::Renderer()
        : m_shader("src/voxel.vert", "src/voxel.frag"), m_max_instances(1000), m_instance_vbo(0) {
        InitializeInstanceBuffers(m_max_instances);
    }

    Renderer::~Renderer() {
        glDeleteBuffers(1, &m_instance_vbo);
    }

    void Renderer::InitializeInstanceBuffers(size_t instance_count) {
        glBindVertexArray(Voxel::GetVao());

        // Transformation matrices
        if (m_instance_vbo == 0) {
            glGenBuffers(1, &m_instance_vbo);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * instance_count, nullptr, GL_DYNAMIC_DRAW);

        for (GLuint i = 0; i < 4; i++) {
            glEnableVertexAttribArray(4 + i);
            glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(4 + i, 1);
        }

        // Unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }




    void Renderer::Render(const Scene& scene) {
        glm::vec4 sky_color = scene.GetSkyColor();

        glClearColor(sky_color.r, sky_color.g, sky_color.b, sky_color.a);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        Camera& camera = scene.GetCamera();
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix();
        m_shader.Use();
        m_shader.SetMat4("view", view);
        m_shader.SetMat4("projection", projection);

        glBindVertexArray(Voxel::GetVao());

        // Determine the total number of instances to render
        size_t total_instances = 0;
        for (const auto& [color, transforms] : scene.GetVoxelMatrices()) {
            total_instances += transforms.size();
        }

        // Resize buffers if the total instances exceed the current max capacity
        if (total_instances > m_max_instances) {
            m_max_instances = total_instances * 1.5;  // Increase by 50% to minimize frequent resizing
            InitializeInstanceBuffers(m_max_instances);
        }

        // Render each voxel group by color
        for (const auto& [color, transforms] : scene.GetVoxelMatrices()) {
            if (transforms.empty()) continue;

            // Update instance buffer data for transformations
            glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, transforms.size() * sizeof(glm::mat4), transforms.data());

            // Set uniform color
            m_shader.SetVec4("color", VoxelColorToColor(color));

            // Render instanced
            glDrawElementsInstanced(GL_TRIANGLES, sizeof(VOXEL_INDICES) / sizeof(VOXEL_INDICES[0]), GL_UNSIGNED_SHORT, 0, transforms.size());
        }


        glBindVertexArray(0);
    }
}
