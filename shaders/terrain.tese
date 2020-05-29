#version 430 core

layout(binding = 0) uniform sampler2D heightmap;

uniform mat4 VIEW_PROJECT;
uniform float amplitude;
uniform float mapscale;

layout(quads, fractional_even_spacing, ccw) in;

out TESSEVAL {
	vec3 position;
	vec2 texcoord;
	float zclipspace;
} tesseval;

void main(void)
{
	vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.y);
	vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.y);
	vec4 pos = mix(p1, p2, gl_TessCoord.x);

	float height = texture(heightmap, mapscale*pos.xz).r;
	pos.y = amplitude * height;

	tesseval.position = pos.xyz;
	tesseval.texcoord = pos.xz;

	gl_Position = VIEW_PROJECT * pos;

	tesseval.zclipspace = gl_Position.z;
}
