#version 330 core

in struct fragment_data
{
    vec3 position;
} fragment;

layout(location=0) out vec4 FragColor;

uniform samplerCube image_texture;



void main()
{
	FragColor = texture(image_texture, fragment.position);
}
