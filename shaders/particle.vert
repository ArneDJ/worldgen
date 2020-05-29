#version 430 core

layout (location = 0) in vec4 position;

uniform mat4 MVP;

out float intensity;
out vec3 fcolor;

void main(void)
{
	intensity = position.w;
	fcolor = position.xyz;
	gl_Position = MVP * vec4(position.xyz, 1.0f);
}
