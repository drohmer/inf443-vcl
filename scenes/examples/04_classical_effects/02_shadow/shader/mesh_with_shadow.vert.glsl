#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 color;
layout (location = 3) in vec2 uv;

out struct fragment_data
{
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
	vec4 fraglight;
	vec3 eye;
} fragment;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 light;
uniform mat4 projection_light;

void main()
{
	fragment.position = vec3(model * vec4(position,1.0));
	fragment.normal   = vec3(model * vec4(normal  ,0.0));
	fragment.color = color;
	fragment.uv = uv;
	fragment.fraglight = projection_light * light * model * vec4(position, 1.0);
	fragment.eye = vec3(inverse(view)*vec4(0,0,0,1.0));
	
	gl_Position = projection * view * model * vec4(position, 1.0);
}
