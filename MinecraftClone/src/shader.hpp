#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "types.hpp"


namespace MC {
    class Shader {
    public:
        Shader(const std::string& vertex_path, const std::string& fragment_path);
        ~Shader();

        void Use() const;

        // Utility functions to set uniform variables in shaders
        void SetBool(const std::string& name, bool value) const;
        void SetInt(const std::string& name, int value) const;
        void SetFloat(const std::string& name, float value) const;
        void SetVec3(const std::string& name, const glm::vec3& value) const;
        void SetVec4(const std::string& name, const glm::vec4& value) const;
        void SetMat4(const std::string& name, const glm::mat4& value) const;
    private:
        // Private utility functions
        std::string ReadShaderFile(const std::string& file_path) const;
        GLuint CompileShader(const std::string& code, GLenum type) const;
        void CheckCompileErrors(GLuint shader, GLenum type) const;

    private:
        u32 m_program_id;
    };
}

#endif
