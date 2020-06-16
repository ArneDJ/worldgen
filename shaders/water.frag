#version 430 core

layout(binding = 0) uniform samplerCube cubemap;
layout(binding = 4) uniform sampler2D normalmap;

uniform vec3 camerapos;
uniform vec3 fogcolor;
uniform float fogfactor;
uniform float time;

out vec4 fcolor;

in TESSEVAL {
	vec3 position;
	vec2 texcoord;
	vec3 incident;
} fragment;

void main(void)
{
	const vec3 lightdirection = vec3(0.5, 0.5, 0.5);
	const vec3 ambient = vec3(0.5, 0.5, 0.5);
	const vec3 lightcolor = vec3(1.0, 1.0, 1.0);

	vec2 D1 = vec2(0.5, 0.5) * (0.1*time);
	vec2 D2 = vec2(-0.5, -0.5) * (0.1*time);

	vec3 normal = texture(normalmap, 0.05*fragment.texcoord + D1).rgb;
	normal += texture(normalmap, 0.05*fragment.texcoord + D2).rgb;

	normal = (normal * 2.0) - 1.0;
	float normaly = normal.y;
	normal.y = normal.z;
	normal.z = normaly;
	normal = normalize(normal);

	const float eta = 0.33;
	vec3 incident = normalize(fragment.incident);

	vec3 reflection = reflect(incident, vec3(0.0, 1.0, 0.0) * normal);
	vec3 refraction = refract(incident, vec3(0.0, 1.0, 0.0) * normal, eta);

	vec4 reflectionColor = texture(cubemap, reflection);
	vec4 refractionColor = texture(cubemap, -refraction);

	float fresnel = 0.7 * pow(max(0.0, 1.0 - dot(-incident, normal)), 0.9);

	fcolor = mix(refractionColor, reflectionColor, fresnel);
	fcolor.rgb *= vec3(0.4, 0.75, 0.8) * 0.7;
	fcolor.a = 0.8;
}

