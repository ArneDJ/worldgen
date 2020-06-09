#version 430 core

layout(vertices = 4) out;

void main(void)
{
	const float tesselation = 16.0;

	if (gl_InvocationID == 0) {
		gl_TessLevelInner[0] = tesselation;
		gl_TessLevelInner[1] = tesselation;
		gl_TessLevelOuter[0] = tesselation;
		gl_TessLevelOuter[1] = tesselation;
		gl_TessLevelOuter[2] = tesselation;
		gl_TessLevelOuter[3] = tesselation;
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
