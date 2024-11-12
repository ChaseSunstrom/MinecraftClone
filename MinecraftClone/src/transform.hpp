#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

namespace MC {
	struct Transform {
		Transform(glm::vec3 pos = glm::vec3(0), glm::vec3 rot = glm::vec3(0), glm::vec3 scale = glm::vec3(1))
			: m_pos(pos), m_rot(rot), m_scale(scale)
		{
			UpdateTransform();
		}

		void Move(glm::vec3 pos)
		{
			m_pos += pos;
			UpdateTransform();
		}

		void Rotate(glm::vec3 rot)
		{
			m_rot += rot;
			UpdateTransform();
		}

		void Scale(glm::vec3 scale)
		{
			m_scale *= scale;
			UpdateTransform();
		}

		glm::vec3 GetPos() const
		{
			return m_pos;
		}

		glm::vec3 GetRot() const
		{
			return m_rot;
		}

		glm::vec3 GetScale() const
		{
			return m_scale;
		}

		const glm::mat4& GetTransform() const
		{
			return m_transform;
		}

		bool operator==(const Transform& other) {
			return m_transform == other.m_transform;
		}

	private:
		void UpdateTransform()
		{
			m_transform = glm::identity<glm::mat4>();

			m_transform = glm::translate(m_transform, m_pos);

			m_transform = glm::rotate(m_transform, m_rot.x, glm::vec3(1, 0, 0));
			m_transform = glm::rotate(m_transform, m_rot.y, glm::vec3(0, 1, 0));
			m_transform = glm::rotate(m_transform, m_rot.z, glm::vec3(0, 0, 1));

			m_transform = glm::scale(m_transform, m_scale);
		}

		glm::vec3 m_pos;
		glm::vec3 m_rot;
		glm::vec3 m_scale;
		glm::mat4 m_transform;
	};
}

#endif
