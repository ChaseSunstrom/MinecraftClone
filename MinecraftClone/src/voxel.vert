#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in mat4 instanceTransform;
layout(location = 6) in vec4 instanceColor;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

uniform mat4 projection;
uniform mat4 view;

void main() {
    FragPos = vec3(instanceTransform * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(instanceTransform))) * aNormal;
    Color = instanceColor;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
