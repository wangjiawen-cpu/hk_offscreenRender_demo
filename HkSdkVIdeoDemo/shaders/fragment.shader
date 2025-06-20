#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D texture_diffuse;
void main()
{
    FragColor = texture(texture_diffuse, vTexCoord);
}
