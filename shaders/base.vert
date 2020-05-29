#version 430 core

#define MAX_JOINT_MATRICES 128

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in ivec4 joints;
layout(location = 4) in vec4 weights;
layout(location = 5) in mat4 instance_model;

uniform mat4 VIEW_PROJECT;
uniform mat4 model;
uniform mat4 u_joint_matrix[MAX_JOINT_MATRICES];
uniform bool skinned;
uniform bool instanced;

out VERTEX {
	vec3 worldpos;
	vec3 normal;
	vec2 texcoord;
} vertex;

void main(void)
{
	vertex.texcoord = texcoord;

	mat4 final_model = model;
	/*
	if (instanced == true) {
		final_model = instance_model;
	}
	*/

	if (skinned == true) {
		mat4 skin_matrix =
		weights.x * u_joint_matrix[int(joints.x)] +
		weights.y * u_joint_matrix[int(joints.y)] +
		weights.z * u_joint_matrix[int(joints.z)] +
		weights.w * u_joint_matrix[int(joints.w)];

		vec4 pos = final_model * skin_matrix * vec4(position, 1.0);
		vertex.normal = normalize(mat3(transpose(inverse(final_model * skin_matrix))) * normal);
		vertex.worldpos = vec4(final_model * skin_matrix * vec4(position, 1.0)).xyz;
		gl_Position = VIEW_PROJECT * pos;
	} else {
		vertex.normal = normalize(mat3(transpose(inverse(final_model))) * normal);
		vertex.worldpos = vec4(final_model * vec4(position, 1.0)).xyz;
		gl_Position = VIEW_PROJECT * final_model * vec4(position, 1.0);
	}
}
