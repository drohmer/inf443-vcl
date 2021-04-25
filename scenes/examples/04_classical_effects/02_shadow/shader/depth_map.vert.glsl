#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 light;
uniform mat4 model;

void main()
{
    gl_Position = projection * light * model * vec4(position, 1.0);
}  