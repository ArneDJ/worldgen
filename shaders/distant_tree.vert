#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 5) in mat4 model;

uniform mat4 VIEW_PROJECT;

out VERTEX {
	vec3 position;
	vec2 texcoord;
} vertex;

void main(void)
{
	vec4 worldpos = model * vec4(position, 1.0);

	vertex.position = worldpos.xyz;
	vertex.texcoord = texcoord;

	gl_Position = VIEW_PROJECT * worldpos;
}
