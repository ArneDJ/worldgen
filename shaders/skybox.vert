#version 430 core

uniform mat4 project, view;

layout(location = 0) in vec3 position;

out vec3 texcoords;

void main()
{
	mat4 boxview = mat4(mat3(view));
	vec4 pos = project * boxview * vec4(position, 1.0);
	texcoords = position;

	gl_Position = pos.xyww;
}  
