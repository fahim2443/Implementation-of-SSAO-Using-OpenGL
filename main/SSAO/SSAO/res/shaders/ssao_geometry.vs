#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 Albedo;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    vec3 viewPos = vec3(view * worldPos);
    vs_out.FragPos = viewPos;
    vs_out.Normal = mat3(transpose(inverse(view * model))) * normal;
    vs_out.Albedo = color;
    gl_Position = projection * vec4(viewPos, 1.0);
}
