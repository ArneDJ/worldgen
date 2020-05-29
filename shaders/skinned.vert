#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in ivec4 joints;
layout(location = 4) in vec4 weights;

layout(binding = 5) uniform samplerBuffer model_matrix_tbo;
layout(binding = 6) uniform samplerBuffer joint_matrix_tbo;

uniform mat4 VIEW_PROJECT;
uniform int JOINT_COUNT;

out VERTEX {
	vec3 worldpos;
	vec3 normal;
	vec2 texcoord;
} vertex;

mat4 fetch_joint_matrix(int joint)
{
	int base_index = gl_InstanceID * 4 * JOINT_COUNT + (4 * joint);

	vec4 col[4];
	col[0] = texelFetch(joint_matrix_tbo, base_index);
	col[1] = texelFetch(joint_matrix_tbo, base_index + 1);
	col[2] = texelFetch(joint_matrix_tbo, base_index + 2);
	col[3] = texelFetch(joint_matrix_tbo, base_index + 3);

	return mat4(col[0], col[1], col[2], col[3]);
}

void main(void)
{
	// fetch the columns from the texture buffer
	vec4 col[4];
	col[0] = texelFetch(model_matrix_tbo, gl_InstanceID * 4);
	col[1] = texelFetch(model_matrix_tbo, gl_InstanceID * 4 + 1);
	col[2] = texelFetch(model_matrix_tbo, gl_InstanceID * 4 + 2);
	col[3] = texelFetch(model_matrix_tbo, gl_InstanceID * 4 + 3);

	// now assemble the four columns into a matrix
	mat4 model = mat4(col[0], col[1], col[2], col[3]);

	mat4 skin = weights.x * fetch_joint_matrix(int(joints.x));
	skin += weights.y * fetch_joint_matrix(int(joints.y));
	skin += weights.z * fetch_joint_matrix(int(joints.z));
	skin += weights.w * fetch_joint_matrix(int(joints.w));

	vec4 pos = model * skin * vec4(position, 1.0);
	vertex.normal = normalize(mat3(transpose(inverse(model * skin))) * normal);
	vertex.worldpos = pos.xyz;
	vertex.texcoord = texcoord;

	gl_Position = VIEW_PROJECT * pos;
}
