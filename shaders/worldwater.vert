#version 430 core

uniform mat4 view, project;

layout(location = 0) in vec3 position;

out VERTEX {
	vec3 position;
	vec2 texcoord;
	vec3 incident;
} vertex;

void main(void)
{
	vec4 worldspace = view * vec4(position, 1.0);

	vertex.position = position;
	vertex.texcoord = vec2(position.x, position.z);
	vertex.incident = inverse(mat3(view)) * vec3(worldspace);

	gl_Position = project * worldspace;
}
