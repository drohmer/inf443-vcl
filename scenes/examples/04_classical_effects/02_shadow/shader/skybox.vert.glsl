#version 330 core

layout (location = 0) in vec3 position;

out struct fragment_data
{
    vec3 position;
} fragment;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	fragment.position = vec3( mat3(model) * position);
	gl_Position = projection * mat4(mat3(view)) * model * vec4(position, 1.0);
}
