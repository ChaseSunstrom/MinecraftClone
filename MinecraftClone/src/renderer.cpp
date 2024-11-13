#include "renderer.hpp"

namespace MC {
    Renderer::Renderer()
        : m_shader("src/voxel.vert", "src/voxel.frag") {
        InitializeBuffers();
    }

    Renderer::~Renderer() {
        glDeleteBuffers(1, &m_instance_vbo);
        glDeleteBuffers(1, &m_chunk_vbo);
        glDeleteBuffers(1, &m_chunk_ebo);
        glDeleteVertexArrays(1, &m_chunk_vao);
    }

    void Renderer::InitializeBuffers() {
        // Initialize VAO, VBO, EBO for chunk rendering
        glGenVertexArrays(1, &m_chunk_vao);
        glGenBuffers(1, &m_chunk_vbo);
        glGenBuffers(1, &m_chunk_ebo);

        glBindVertexArray(m_chunk_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_chunk_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(VOXEL_VERTICES), VOXEL_VERTICES, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_chunk_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VOXEL_INDICES), VOXEL_INDICES, GL_STATIC_DRAW);

        // Set up vertex attributes
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glBindVertexArray(0);
    }

    void Renderer::Render(ThreadPool& tp, Scene& scene) {
        glm::vec4 sky_color = scene.GetSkyColor();
        glClearColor(sky_color.r, sky_color.g, sky_color.b, sky_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Camera& camera = scene.GetCamera();
        Frustum& camera_frustum = camera.GetFrustum();
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix();
        glm::mat4 viewProj = projection * view;

        m_shader.Use();
        m_shader.SetMat4("view", view);
        m_shader.SetMat4("projection", projection);

        // Update frustum
        camera_frustum.Update(viewProj);

        auto& chunks = scene.GetChunks();

        // Now proceed to rendering
        for (auto& [chunk_pos, chunk] : chunks) {
            // Skip chunks that are not visible
            glm::vec3 chunk_world_pos = glm::vec3(chunk_pos * Chunk::CHUNK_SIZE);
            glm::vec3 chunk_min = chunk_world_pos;
            glm::vec3 chunk_max = chunk_world_pos + glm::vec3(Chunk::CHUNK_SIZE);

            if (!camera_frustum.IsBoxVisible(chunk_min, chunk_max)) {
                continue; // Chunk is outside the view frustum
            }

            // Ensure mesh data is uploaded
            if (!chunk->IsMeshDataUploaded()) {
                continue; // Skip if mesh data is not ready
            }

            // Set up model matrix for the chunk
            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk_world_pos);
            m_shader.SetMat4("model", model);

            // Bind chunk-specific VAO
            glBindVertexArray(chunk->GetVAO());

            // Draw the chunk
            glDrawElements(GL_TRIANGLES, chunk->GetIndexCount(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
        }
    }

}
