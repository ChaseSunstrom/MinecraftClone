#include "renderer.hpp"

#include <GLFW/glfw3.h>

namespace MC {
    Renderer::Renderer()
        : m_lit_shader("src/lit.vert", "src/lit.frag"),
          m_unlit_shader("src/unlit.vert", "src/unlit.frag"),
		  m_enable_lighting(true) {
    }

    void Renderer::EnableLighting(bool enable)
    {
        m_enable_lighting = enable;
    }

    bool Renderer::IsLightingEnabled() const
    {
        return m_enable_lighting;
    }



    void Renderer::Render(ThreadPool& tp, Scene& scene) {

        Camera& camera = scene.GetCamera();
        Frustum& camera_frustum = camera.GetFrustum();
        const Sun& sun = scene.GetSun();
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjectionMatrix();
        glm::mat4 view_proj = projection * view;

        Shader& current_shader = m_enable_lighting ? m_lit_shader : m_unlit_shader;

        current_shader.Use();
        current_shader.SetMat4("view", view);
        current_shader.SetMat4("projection", projection);
    	current_shader.SetVec3("viewPos", camera.GetPosition());


        f32 time = glfwGetTime();
        f32 sun_angle = time * 0.05f;

        glm::vec3 light_direction = glm::normalize(glm::vec3(
            cos(sun_angle),
            sin(sun_angle),
            0.0f
        ));

        if (m_enable_lighting) {


            // Adjust light color based on sun's position
            glm::vec3 light_color;
            glm::vec3 ambient_light_color;

            if (light_direction.y > 0.0f) {
                // Daytime
                light_color = glm::vec3(1.0f, 0.95f, 0.8f);
                ambient_light_color = glm::vec3(0.3f, 0.3f, 0.35f);
            }
            else {
                // Nighttime
                light_color = glm::vec3(0.0f, 0.0f, 0.0f);
                ambient_light_color = glm::vec3(0.05f, 0.05f, 0.1f);
            }


            f32 sun_height = light_direction.y;

            sun_height = glm::clamp(sun_height, -1.0f, 1.0f);

            // Calculate light intensity
            f32 intensity = glm::max(sun_height, 0.0f);

            // Adjust light color based on sun height
            glm::vec4 sky_color;
            glm::vec4 day_sky_color = glm::vec4(0.5f, 0.7f, 0.9f, 1.0f);
            glm::vec4 sunset_sky_color = glm::vec4(1.0f, 0.5f, 0.3f, 1.0f);
            glm::vec4 night_sky_color = glm::vec4(0.05f, 0.05f, 0.1f, 1.0f);
            glm::vec3 day_light_color = glm::vec3(1.0f, 0.95f, 0.8f);
            glm::vec3 sunset_light_color = glm::vec3(1.0f, 0.5f, 0.3f);
            glm::vec3 night_light_color = glm::vec3(0.0f, 0.0f, 0.0f);

            if (sun_height > 0.0f) {
                // Daytime to sunset transition
                light_color = glm::mix(sunset_light_color, day_light_color, sun_height);
                ambient_light_color = glm::vec3(0.3f) * intensity;
                sky_color = glm::mix(sunset_sky_color, day_sky_color, sun_height);
            }
            else {
                // Sunset to night transition
                light_color = night_light_color;
                ambient_light_color = glm::vec3(0.05f);
                sky_color = glm::mix(night_sky_color, sunset_sky_color, sun_height + 1.0f);
            }

            m_lit_shader.SetVec3("lightDirection", light_direction);
            m_lit_shader.SetVec3("lightColor", light_color);
            m_lit_shader.SetVec3("ambientLightColor", ambient_light_color);

            scene.SetSkyColor(sky_color);

            glClearColor(sky_color.r, sky_color.g, sky_color.b, sky_color.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else
        {
            glm::vec4 day_sky_color = glm::vec4(0.5f, 0.7f, 0.9f, 1.0f);

            scene.SetSkyColor(day_sky_color);

            glClearColor(day_sky_color.r, day_sky_color.g, day_sky_color.b, day_sky_color.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        camera_frustum.Update(view_proj);

        auto& chunks = scene.GetChunks();

        for (auto& [chunk_pos, chunk] : chunks) {
            glm::vec3 chunk_world_pos = glm::vec3(chunk_pos * Chunk::CHUNK_SIZE);
            glm::vec3 chunk_min = chunk_world_pos;
            glm::vec3 chunk_max = chunk_world_pos + glm::vec3(Chunk::CHUNK_SIZE);

            // Skip chunks that are not visible
            if (!camera_frustum.IsBoxVisible(chunk_min, chunk_max)) {
                continue;
            }

            if (!chunk->IsMeshDataUploaded()) {
                continue; // Skip if mesh data is not ready
            }

            glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk_world_pos);
            current_shader.SetMat4("model", model);

            glBindVertexArray(chunk->GetVAO());

            glDrawElements(GL_TRIANGLES, chunk->GetIndexCount(), GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
        }

        if (m_enable_lighting)
        {
            RenderSun(sun, camera, light_direction);
        }
    }

    void Renderer::RenderSun(const Sun& sun, const Camera& camera, const glm::vec3& light_direction) {
        glm::vec3 sun_position = light_direction * 2500.0f; // Position sun far away
        glm::mat4 model = glm::translate(glm::mat4(1.0f), sun_position);
        model = glm::scale(model, glm::vec3(150.0f)); // Scale the sun cube

        m_unlit_shader.Use();
        m_unlit_shader.SetMat4("model", model);
        m_unlit_shader.SetMat4("view", camera.GetViewMatrix());
        m_unlit_shader.SetMat4("projection", camera.GetProjectionMatrix());

        glBindVertexArray(sun.GetVAO());
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

}
