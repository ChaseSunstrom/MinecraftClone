#include "frustum.hpp"
#include "types.hpp"
#include <array>

namespace MC {
    void Frustum::Update(const glm::mat4& view_proj) {
        // Left plane
        m_planes[0] = glm::vec4(
            view_proj[0][3] + view_proj[0][0],
            view_proj[1][3] + view_proj[1][0],
            view_proj[2][3] + view_proj[2][0],
            view_proj[3][3] + view_proj[3][0]);

        // Right plane
        m_planes[1] = glm::vec4(
            view_proj[0][3] - view_proj[0][0],
            view_proj[1][3] - view_proj[1][0],
            view_proj[2][3] - view_proj[2][0],
            view_proj[3][3] - view_proj[3][0]);

        // Bottom plane
        m_planes[2] = glm::vec4(
            view_proj[0][3] + view_proj[0][1],
            view_proj[1][3] + view_proj[1][1],
            view_proj[2][3] + view_proj[2][1],
            view_proj[3][3] + view_proj[3][1]);

        // Top plane
        m_planes[3] = glm::vec4(
            view_proj[0][3] - view_proj[0][1],
            view_proj[1][3] - view_proj[1][1],
            view_proj[2][3] - view_proj[2][1],
            view_proj[3][3] - view_proj[3][1]);

        // Near plane
        m_planes[4] = glm::vec4(
            view_proj[0][3] + view_proj[0][2],
            view_proj[1][3] + view_proj[1][2],
            view_proj[2][3] + view_proj[2][2],
            view_proj[3][3] + view_proj[3][2]);

        // Far plane
        m_planes[5] = glm::vec4(
            view_proj[0][3] - view_proj[0][2],
            view_proj[1][3] - view_proj[1][2],
            view_proj[2][3] - view_proj[2][2],
            view_proj[3][3] - view_proj[3][2]);

        // Normalize the planes
        for (i32 i = 0; i < 6; i++) {
            f32 length = glm::length(glm::vec3(m_planes[i]));
            m_planes[i] /= length;
        }
    }

    bool Frustum::IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const {
        // Test the bounding box against each plane
        for (i32 i = 0; i < 6; ++i) {
            const glm::vec4& plane = m_planes[i];

            glm::vec3 positive_vertex = min;
            glm::vec3 negative_vertex = max;

            if (plane.x >= 0) {
                positive_vertex.x = max.x;
                negative_vertex.x = min.x;
            }
            if (plane.y >= 0) {
                positive_vertex.y = max.y;
                negative_vertex.y = min.y;
            }
            if (plane.z >= 0) {
                positive_vertex.z = max.z;
                negative_vertex.z = min.z;
            }

            if (glm::dot(glm::vec3(plane), positive_vertex) + plane.w < 0) {
                return false; // Outside the frustum
            }
        }
        return true; // Inside or intersecting the frustum
    }
}
