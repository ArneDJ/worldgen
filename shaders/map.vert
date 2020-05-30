#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 VIEW_PROJECT;
uniform mat4 model;

out VERTEX {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
} vertex;

void main(void)
{
	vertex.texcoord = texcoord;
	vertex.normal = normalize(mat3(transpose(inverse(model))) * normal);
	vertex.position = vec4(model * vec4(position, 1.0)).xyz;
	gl_Position = VIEW_PROJECT * model * vec4(position, 1.0);
}
