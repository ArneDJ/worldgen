#version 430 core

uniform samplerCube cubemap;

out vec4 fcolor;

in vec3 texcoords;

void main()
{    
	fcolor = texture(cubemap, texcoords);
}
