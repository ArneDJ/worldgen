#version 430 core

out vec4 color;

layout(binding = 0) uniform sampler2D basemap;
layout(binding = 1) uniform sampler2D heightmap;

in VERTEX {
	vec3 position;
	vec3 normal;
	vec2 texcoord;
} fragment;

void main(void)
{
	//float height = texture(heightmap, fragment.texcoord).r;
	color = texture(basemap, fragment.texcoord);
	//color.rgb *= height;
	//color = vec4(sin(100.0*fragment.texcoord.x), 0.0, 0.0, 1.0);
}
