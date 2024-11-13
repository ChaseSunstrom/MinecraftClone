#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include <glm/glm.hpp>

namespace MC {
    class Frustum {
    public:
        // Update the frustum planes based on the view-projection matrix
        void Update(const glm::mat4& view_proj);

        // Check if a bounding box is visible within the frustum
        bool IsBoxVisible(const glm::vec3& min, const glm::vec3& max) const;

    private:
        // Frustum planes: left, right, bottom, top, near, far
        glm::vec4 m_planes[6];
    };
}

#endif // FRUSTUM_HPP
