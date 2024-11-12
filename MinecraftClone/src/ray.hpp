#ifndef RAY_HPP
#define RAY_HPP

#include <glm/glm.hpp>

namespace MC {
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;

        Ray(const glm::vec3& o = glm::vec3(0.0f),
            const glm::vec3& d = glm::vec3(0.0f, 0.0f, -1.0f))
            : origin(o), direction(glm::normalize(d)) {}
    };
}

#endif // RAY_HPP
