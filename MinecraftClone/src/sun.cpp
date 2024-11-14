#include "sun.hpp"

#include <GL/glew.h>

namespace MC
{
	void Sun::Initialize()
	{

		glGenVertexArrays(1, &m_sun_vao);
		glGenBuffers(1, &m_sun_vbo);
		glGenBuffers(1, &m_sun_ebo);

		glBindVertexArray(m_sun_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_sun_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SUN_VERTICES), SUN_VERTICES, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sun_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(SUN_INDICES), SUN_INDICES, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(
			2,
			4,
			GL_FLOAT,
			GL_FALSE,
			sizeof(Vertex),
			(void*)offsetof(Vertex, color)
		);

		glBindVertexArray(0);
	}

	Sun::~Sun()
	{
		glDeleteBuffers(1, &m_sun_vbo);
		glDeleteBuffers(1, &m_sun_ebo);
		glDeleteVertexArrays(1, &m_sun_vao);
	}

}
