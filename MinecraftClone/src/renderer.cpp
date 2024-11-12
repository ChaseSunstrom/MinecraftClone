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
        glDeleteBuffers(1, &m_color_vbo);
    }

    void Renderer::InitializeInstanceBuffers(size_t instance_count) {
        // Bind the VAO before setting up instance attributes
        glBindVertexArray(Voxel::GetVao());

        // Generate or resize the instance VBO for transformation matrices
        if (m_instance_vbo == 0) {
            glGenBuffers(1, &m_instance_vbo);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * instance_count, nullptr, GL_DYNAMIC_DRAW);

        // Set up instance attribute pointers for transformation matrix
        for (GLuint i = 0; i < 4; i++) {
            glEnableVertexAttribArray(2 + i);
            glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(2 + i, 1);
        }

        // Generate or resize the instance VBO for colors
        if (m_color_vbo == 0) {
            glGenBuffers(1, &m_color_vbo);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * instance_count, nullptr, GL_DYNAMIC_DRAW);

        // Color attribute
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
        glVertexAttribDivisor(6, 1);

        // Unbind the VAO and VBO for safety
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }


    void Renderer::Render(const Scene& scene) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec4 sky_color = scene.GetSkyColor();

        glClearColor(sky_color.r, sky_color.g, sky_color.b, sky_color.a);

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

            // Update instance buffer data for colors
            std::vector<glm::vec4> colors(transforms.size(), VoxelColorToColor(color));
            glBindBuffer(GL_ARRAY_BUFFER, m_color_vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(glm::vec4), colors.data());

            // Render instanced
            glDrawElementsInstanced(GL_TRIANGLES, sizeof(VOXEL_INDICES) / sizeof(VOXEL_INDICES[0]), GL_UNSIGNED_SHORT, 0, transforms.size());
        }

        glBindVertexArray(0);
    }
}
