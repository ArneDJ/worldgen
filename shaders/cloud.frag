#version 430 core

layout(binding = 0) uniform sampler3D cloudmap;

layout(location = 0) out vec4 color;

uniform float time;

in VERTEX {
	vec3 position;
} fragment;

void main(void)
{
	float wind = 0.01 * time;
	color = texture(cloudmap, vec3(fragment.position.x+wind, fragment.position.y+wind, fragment.position.z+wind)).rrrr;

	color.rgb = vec3(1.0, 0.9, 0.9) * (1.0 - color.a);
}
