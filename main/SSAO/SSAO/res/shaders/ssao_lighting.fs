#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssao;
uniform Light light;
uniform vec3 viewPos;

void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 Albedo = texture(gAlbedo, TexCoords).rgb;
    float ao = texture(ssao, TexCoords).r;

    vec3 ambient = 0.25 * Albedo * ao;

    vec3 lightDir = normalize(light.Position - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = diff * Albedo * light.Color;

    vec3 viewDir = normalize(-FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * light.Color * 0.4;

    vec3 color = ambient + diffuse + specular;
    FragColor = vec4(color, 1.0);
}
