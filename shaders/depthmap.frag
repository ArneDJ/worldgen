#version 430 core

layout(binding = 0) uniform sampler2D diffuse;

layout(location = 0) out vec4 color;

in VERTEX {
	vec2 texcoord;
} fragment;

void main(void)
{
	float alpha = texture(diffuse, fragment.texcoord).a;
	if (alpha < 0.5) { discard; }
	color = vec4(1.0);
}
