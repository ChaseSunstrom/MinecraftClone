#ifndef VOXEL_HIT_INFO_HPP
#define VOXEL_HIT_INFO_HPP

#include "voxel.hpp"
#include "voxel_face.hpp"

namespace MC {
    struct VoxelHitInfo {
        Voxel voxel;
        VoxelFace face;
    };
}

#endif // VOXEL_HIT_INFO_HPP
