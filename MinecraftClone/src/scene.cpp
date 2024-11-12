#include "scene.hpp"

namespace MC {
    Scene::Scene(EventHandler& event_handler)
        :
        m_event_handler(event_handler),
        m_camera(std::make_unique<Camera>()),
        m_sky_color(glm::vec4(0.2f, 0.3f, 0.4f, 1.0f)) {
        Camera& camera = *m_camera;
        m_event_handler.SubscribeToEvent<WindowResizedEvent>([&camera](EventPtr<WindowResizedEvent> event) {
            camera.OnWindowResize(event);
            });
    }

    Scene::~Scene() {
        Voxel::CleanupStaticBuffers();
    }

    void Scene::InitializeScene() {
        Voxel::InitializeStaticBuffers();
    }

    void Scene::SetSkyColor(const glm::vec4& sky_color) {
        m_sky_color = sky_color;
    }

    glm::vec4 Scene::GetSkyColor() const {
        return m_sky_color;
    }

    void Scene::InsertVoxel(const Voxel& voxel) {
        u32 id = voxel.GetID();
        m_voxels.emplace(id, voxel);
        m_voxel_transforms[voxel.GetVoxelColor()].push_back(voxel.GetTransform());
        m_voxel_matrices[voxel.GetVoxelColor()].push_back(voxel.GetTransform().GetTransform());
    }

    void Scene::RemoveVoxel(u32 voxel_id) {
        auto it = m_voxels.find(voxel_id);
        if (it == m_voxels.end()) {
            // Voxel ID not found
            return;
        }

        Voxel voxel_to_remove = it->second;

        // Remove from m_voxel_transforms
        auto& transforms = m_voxel_transforms[voxel_to_remove.GetVoxelColor()];
        transforms.erase(std::remove(transforms.begin(), transforms.end(), voxel_to_remove.GetTransform()), transforms.end());

        // Remove from m_voxel_matrices
        auto& matrices = m_voxel_matrices[voxel_to_remove.GetVoxelColor()];
        matrices.erase(std::remove(matrices.begin(), matrices.end(), voxel_to_remove.GetTransform().GetTransform()), matrices.end());

        // Remove from m_voxels
        m_voxels.erase(it);
    }

    void Scene::UpdateVoxel(const Voxel& updated_voxel) {
        u32 voxel_id = updated_voxel.GetID();
        auto it = m_voxels.find(voxel_id);
        if (it == m_voxels.end()) {
            // Voxel ID not found
            return;
        }

        Voxel& voxel = it->second;

        // Remove old transform and matrix
        auto& old_transforms = m_voxel_transforms[voxel.GetVoxelColor()];
        old_transforms.erase(std::remove(old_transforms.begin(), old_transforms.end(), voxel.GetTransform()), old_transforms.end());

        auto& old_matrices = m_voxel_matrices[voxel.GetVoxelColor()];
        old_matrices.erase(std::remove(old_matrices.begin(), old_matrices.end(), voxel.GetTransform().GetTransform()), old_matrices.end());

        // Update voxel properties
        voxel = updated_voxel;

        // Re-insert updated transform and matrix
        m_voxel_transforms[voxel.GetVoxelColor()].push_back(voxel.GetTransform());
        m_voxel_matrices[voxel.GetVoxelColor()].push_back(voxel.GetTransform().GetTransform());
    }

    const std::unordered_map<VoxelColor, std::vector<Transform>>& Scene::GetVoxelTransforms() const {
        return m_voxel_transforms;
    }

    const std::unordered_map<VoxelColor, std::vector<glm::mat4>>& Scene::GetVoxelMatrices() const {
        return m_voxel_matrices;
    }

    const std::unordered_map<u32, Voxel>& Scene::GetVoxels() const {
        return m_voxels;
    }

    std::optional<Voxel> Scene::GetVoxel(u64 id) const {
        auto it = m_voxels.find(id);

        if (it != m_voxels.end())
            return std::make_optional<Voxel>((*it).second);

        return std::nullopt;
    }

    Camera& Scene::GetCamera() const {
        return *m_camera;
    }
}
