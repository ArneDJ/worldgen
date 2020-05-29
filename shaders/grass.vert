#version 430 core

layout(location = 0) in vec2 coordinate;

void main(void)
{
	gl_Position = vec4(coordinate.x, 0.0, coordinate.y, 1.0);
}
