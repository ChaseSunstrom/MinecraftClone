#include "shader.hpp"
#include "log.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace MC {

	Shader::Shader(const std::string& vertex_path, const std::string& fragment_path) {
		// Read shader code from files
		std::string vertex_code = ReadShaderFile(vertex_path);
		std::string fragment_code = ReadShaderFile(fragment_path);

		// Compile shaders
		GLuint vertex_shader = CompileShader(vertex_code, GL_VERTEX_SHADER);
		GLuint fragment_shader = CompileShader(fragment_code, GL_FRAGMENT_SHADER);

		// Link shaders into shader program
		m_program_id = glCreateProgram();
		glAttachShader(m_program_id, vertex_shader);
		glAttachShader(m_program_id, fragment_shader);
		glLinkProgram(m_program_id);
		CheckCompileErrors(m_program_id, GL_PROGRAM);

		// Delete shaders after linking
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}

	Shader::~Shader() {
		glDeleteProgram(m_program_id);
	}

	void Shader::Use() const {
		glUseProgram(m_program_id);
	}

	void Shader::SetBool(const std::string& name, bool value) const {
		glUniform1i(glGetUniformLocation(m_program_id, name.c_str()), static_cast<i32>(value));
	}

	void Shader::SetInt(const std::string& name, i32 value) const {
		glUniform1i(glGetUniformLocation(m_program_id, name.c_str()), value);
	}

	void Shader::SetFloat(const std::string& name, f32 value) const {
		glUniform1f(glGetUniformLocation(m_program_id, name.c_str()), value);
	}

	void Shader::SetVec3(const std::string& name, const glm::vec3& value) const {
		glUniform3fv(glGetUniformLocation(m_program_id, name.c_str()), 1, glm::value_ptr(value));
	}

	void Shader::SetVec4(const std::string& name, const glm::vec4& value) const {
		glUniform4fv(glGetUniformLocation(m_program_id, name.c_str()), 1, glm::value_ptr(value));
	}

	void Shader::SetMat4(const std::string& name, const glm::mat4& value) const {
		glUniformMatrix4fv(glGetUniformLocation(m_program_id, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}

	std::string Shader::ReadShaderFile(const std::string& file_path) const {
		std::ifstream file(file_path);
		if (!file) {
			LOG_FATAL("Could not open shader file " << file_path);
			return "";
		}
		std::stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	GLuint Shader::CompileShader(const std::string& code, GLenum type) const {
		GLuint shader = glCreateShader(type);
		const char* shader_code = code.c_str();
		glShaderSource(shader, 1, &shader_code, nullptr);
		glCompileShader(shader);
		CheckCompileErrors(shader, type);
		return shader;
	}

	void Shader::CheckCompileErrors(GLuint shader, GLenum type) const {
		GLint success;
		GLchar info_log[1024];
		if (type != GL_PROGRAM) {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 1024, nullptr, info_log);
				LOG_FATAL("SHADER_COMPILATION_ERROR of type: "
					<< (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
					<< "\n" << info_log << "\n -- --------------------------------------------------- -- "
					);
			}
		}
		else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, nullptr, info_log);
				LOG_FATAL("PROGRAM_LINKING_ERROR of type: PROGRAM\n"
					<< info_log << "\n -- --------------------------------------------------- -- "
					);
			}
		}
	}
}