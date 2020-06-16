#version 430 core

layout(binding = 1) uniform sampler2D heightmap;
layout(binding = 2) uniform sampler2D rivermap;

uniform mat4 view, project;
uniform mat4 shadownear;
uniform mat4 shadowmiddle;
uniform mat4 shadowfar;
uniform float amplitude;
uniform float mapscale;

layout(quads, fractional_even_spacing, ccw) in;

out TESSEVAL {
	vec3 position;
	vec2 texcoord;
	vec3 incident;
} tesseval;

void main(void)
{
	vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.y);
	vec4 p2 = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, gl_TessCoord.y);
	vec4 pos = mix(p1, p2, gl_TessCoord.x);

	pos.y = 10.0;

	float river = texture(rivermap, mapscale*pos.xz).r;
	river = smoothstep(0.0, 0.5, river);
	float height = texture(heightmap, mapscale*pos.xz).r;
	pos.y = mix(pos.y, amplitude*height+0.25, river);

	vec4 vertex = view * pos;

	tesseval.position = pos.xyz;
	tesseval.texcoord = pos.xz;
	tesseval.incident = inverse(mat3(view)) * vec3(vertex);

	gl_Position = project * vertex;
}
