#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec4 VertexColor;

out vec4 FragColor;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientLightColor;
uniform vec3 viewPos; // Camera position

void main()
{
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(-lightDirection);

        // Ambient component
        vec3 ambient = ambientLightColor * VertexColor.rgb;

        // Diffuse component
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor * VertexColor.rgb;

        // Specular component
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // Shininess factor
        vec3 specular = spec * lightColor;

        // Combine results
        vec3 result = ambient + diffuse + specular;

        FragColor = vec4(result, VertexColor.a);
}
