#ifndef HASH_HPP
#define HASH_HPP

#include <glm/glm.hpp>
#include <functional>
#include "types.hpp"

namespace std {
    template <>
    struct hash<glm::ivec3> {
        std::size_t operator()(const glm::ivec3& key) const {
            return ((std::hash<i32>()(key.x) ^
                (std::hash<i32>()(key.y) << 1)) >> 1) ^
                (std::hash<i32>()(key.z) << 1);
        }
    };
}

#endif // HASH_HPP
