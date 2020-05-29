#version 430 core

layout(binding = 0) uniform sampler2D basemap;

uniform vec3 RGB;

out vec4 color;

in VERTEX {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
} fragment;

void main(void)
{
	const vec3 lightdirection = vec3(0.2, 0.5, 0.5);
	const vec3 ambient = vec3(0.5, 0.5, 0.5);
	const vec3 lightcolor = vec3(1.0, 0.9, 0.8);

	//color = vec4(1.0, 0.5, 0.1, 1.0);
	color = vec4(RGB, 1.0);

	float diffuse = max(0.0, dot(fragment.normal, lightdirection));
	vec3 scatteredlight = ambient + lightcolor * diffuse;
	color.rgb = min(color.rgb * scatteredlight, vec3(1.0));
}

