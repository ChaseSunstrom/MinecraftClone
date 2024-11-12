#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

out vec4 FragColor;

void main() {
    FragColor = Color;
}
