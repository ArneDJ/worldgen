#version 430 core

layout (location = 0) out vec4 color;

in float intensity;
in vec3 fcolor;

void main(void)
{
	//color = mix(vec4(1.f, 1.f, 1.f, 1.f), vec4(0.2f, 0.05f, 0.0f, 1.f), intensity);
	color = vec4(fcolor, 1.0);
}
