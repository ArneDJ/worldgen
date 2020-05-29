#version 430

uniform mat4 VIEW_PROJECT;
uniform mat4 tc_rotate;

layout(location = 0) in vec3 position;

out VERTEX {
	vec3 position;
} vertex;

void main(void)
{
	float tiling = 1.0 / 2048.0;

	vec4 worldposition = vec4(position, 1.0);

	vertex.position = tiling * worldposition.stp;
	gl_Position = VIEW_PROJECT * worldposition;
}
