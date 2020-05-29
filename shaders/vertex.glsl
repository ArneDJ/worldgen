#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 project, view, model;

out VERTEX {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
} vertex;

void main(void)
{
	vec4 worldpos = model * vec4(position, 1.0);

	vertex.position = worldpos.xyz;
	//vertex.normal = normal;
	vertex.normal = normalize(mat3(transpose(inverse(model))) * normal);
	vertex.texcoord = texcoord;

	gl_Position = project * view * worldpos;
}
