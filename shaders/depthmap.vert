#version 430

#define MAX_JOINT_MATRICES 128

uniform mat4 VIEW_PROJECT;
uniform mat4 model;
uniform mat4 u_joint_matrix[MAX_JOINT_MATRICES];
uniform bool skinned;
uniform bool instanced;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in ivec4 joints;
layout(location = 4) in vec4 weights;
layout(location = 5) in mat4 instance_matrix;

out VERTEX {
	vec2 texcoord;
} vertex;

void main(void)
{
	vertex.texcoord = texcoord;
	if (instanced == true) {
		gl_Position = VIEW_PROJECT * instance_matrix * vec4(position, 1.0);
	} else {
		if (skinned == true) {
		mat4 skin_matrix =
		weights.x * u_joint_matrix[int(joints.x)] +
		weights.y * u_joint_matrix[int(joints.y)] +
		weights.z * u_joint_matrix[int(joints.z)] +
		weights.w * u_joint_matrix[int(joints.w)];

		vec4 pos = model * skin_matrix * vec4(position, 1.0);
		gl_Position = VIEW_PROJECT * pos;
		} else {
		gl_Position = VIEW_PROJECT * model * vec4(position, 1.0);
		}
	}
}
