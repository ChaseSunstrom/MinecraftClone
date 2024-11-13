#include "renderer.hpp"
#include "voxel.hpp"
#include <GL/glew.h>

namespace MC {
    Renderer::Renderer()
        : m_shader("src/voxel.vert", "src/voxel.frag"), m_max_instances(1000), m_instance_vbo(0) {
        InitializeInstanceBuffers(m_max_instances);
        InitializeFaceBuffers();
    }

    Renderer::~Renderer() {
        glDeleteBuffers(1, &m_instance_vbo);
        glDeleteBuffers(6, m_face_vbos.data());
        glDeleteVertexArrays(6, m_face_vaos.data());
    }

    void Renderer::UpdateInstanceData(const Scene& scene) {
        m_face_instance_matrices.clear();

        for (const auto& [voxel_id, voxel] : scene.GetVoxels()) {
            for (int face = 0; face < 6; ++face) {
                if (voxel.IsFaceVisible(static_cast<Voxel::FaceIndex>(face))) {
                    m_face_instance_matrices[face][voxel.GetVoxelColor()].push_back(voxel.GetTransform().GetTransform());
                }
            }
        }
    }

    void Renderer::InitializeFaceBuffers() {
        // Vertex data for a single face quad in each direction
        const Vertex FACE_VERTICES[6][4] = {
            // POS_X Face (+X)
            {
                {{1, 0, 0}, {1, 0, 0}},
                {{1, 1, 0}, {1, 0, 0}},
                {{1, 1, 1}, {1, 0, 0}},
                {{1, 0, 1}, {1, 0, 0}}
            },
            // NEG_X Face (-X)
            {
                {{0, 0, 1}, {-1, 0, 0}},
                {{0, 1, 1}, {-1, 0, 0}},
                {{0, 1, 0}, {-1, 0, 0}},
                {{0, 0, 0}, {-1, 0, 0}}
            },
            // POS_Y Face (+Y)
            {
                {{0, 1, 1}, {0, 1, 0}},
                {{1, 1, 1}, {0, 1, 0}},
                {{1, 1, 0}, {0, 1, 0}},
                {{0, 1, 0}, {0, 1, 0}}
            },
            // NEG_Y Face (-Y)
            {
                {{0, 0, 0}, {0, -1, 0}},
                {{1, 0, 0}, {0, -1, 0}},
                {{1, 0, 1}, {0, -1, 0}},
                {{0, 0, 1}, {0, -1, 0}}
            },
            // POS_Z Face (+Z)
            {
                {{0, 0, 1}, {0, 0, 1}},
                {{1, 0, 1}, {0, 0, 1}},
                {{1, 1, 1}, {0, 0, 1}},
                {{0, 1, 1}, {0, 0, 1}}
            },
            // NEG_Z Face (-Z)
            {
                {{0, 1, 0}, {0, 0, -1}},
                {{1, 1, 0}, {0, 0, -1}},
                {{1, 0, 0}, {0, 0, -1}},
                {{0, 0, 0}, {0, 0, -1}}
            }
        };

        glGenVertexArrays(6, m_face_vaos.data());
        glGenBuffers(6, m_face_vbos.data());
        glGenBuffers(6, m_face_ebos.data());

        for (i32 i = 0; i < 6; ++i) {
            glBindVertexArray(m_face_vaos[i]);
            glBindBuffer(GL_ARRAY_BUFFER, m_face_vbos[i]);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, FACE_VERTICES[i], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_face_ebos[i]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(VOXEL_INDICES) / sizeof(VOXEL_INDICES[0]), VOXEL_INDICES, GL_STATIC_DRAW);

            // Set up vertex attributes within the VAO
            // Position attribute
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

            // Normal attribute
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));


            // Instance matrix attributes
            glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
            for (GLuint j = 0; j < 4; j++) {
                glEnableVertexAttribArray(4 + j);
                glVertexAttribPointer(4 + j, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * j));
                glVertexAttribDivisor(4 + j, 1);
            }

            // Unbind VAO (good practice)
            glBindVertexArray(0);
        }

        // Unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
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

        UpdateInstanceData(scene);

        glm::vec4 sky_color = scene.GetSkyColor();
        glClearColor(sky_color.r, sky_color.g, sky_color.b, sky_color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Camera& camera = scene.GetCamera();
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix();
        m_shader.Use();
        m_shader.SetMat4("view", view);
        m_shader.SetMat4("projection", projection);

        // For each face direction
        for (i32 face = 0; face < 6; ++face) {
            // Access precomputed instance data
            auto& color_to_matrices = m_face_instance_matrices[face];

            // If no instances for this face, skip
            if (color_to_matrices.empty()) continue;

            // Bind the VAO for the current face
            glBindVertexArray(m_face_vaos[face]);

            // For each color, draw the instances
            for (const auto& [color, matrices] : color_to_matrices) {
                if (matrices.empty()) continue;

                // Update instance buffer with transformation matrices
                glBindBuffer(GL_ARRAY_BUFFER, m_instance_vbo);
                glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(glm::mat4), matrices.data(), GL_STATIC_DRAW);

                // Set voxel color
                m_shader.SetVec4("color", VoxelColorToColor(color));

                // Draw instanced quads
                glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, matrices.size());
            }

            // Unbind VAO
            glBindVertexArray(0);
        }
    }


}
