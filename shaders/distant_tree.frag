#version 430 core

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D normalmap;
layout(binding = 2) uniform sampler2D occlusmap;
layout(binding = 4) uniform sampler2D basemap;

uniform float mapscale;
uniform vec3 camerapos;
uniform vec3 fogcolor;
uniform float fogfactor;

out vec4 color;

in VERTEX {
	vec3 position;
	vec2 texcoord;
} fragment;

vec3 fog(vec3 c, float dist, float height)
{
	float de = fogfactor * smoothstep(0.0, 3.3, 1.0 - height);
	float di = fogfactor * smoothstep(0.0, 5.5, 1.0 - height);
	float extinction = exp(-dist * de);
	float inscattering = exp(-dist * di);

	return c * extinction + fogcolor * (1.0 - inscattering);
}

void main(void)
{
	const vec3 lightdirection = vec3(0.5, 0.5, 0.5);
	const vec3 ambient = vec3(0.5, 0.5, 0.5);
	const vec3 lightcolor = vec3(1.0, 1.0, 1.0);
	const vec3 viewspace = vec3(distance(fragment.position.x, camerapos.x), distance(fragment.position.y, camerapos.y), distance(fragment.position.z, camerapos.z));

	color = texture(basemap, fragment.texcoord);
	if(color.a < 0.5) { discard; }
	color.a = 1.0;

	color.rgb = mix(vec3(0.34, 0.5, 0.09), color.rgb, 0.5);
	color.rgb *= 0.8;

	//vec3 normal = vec3(0.0, 1.0, 0.0);
	vec3 normal = texture(normalmap, mapscale * fragment.position.xz).rgb;
	normal = (normal * 2.0) - 1.0;
	float diffuse = max(0.0, dot(normal, lightdirection));
	diffuse = clamp(diffuse, 0.2, 1.0);
	
	color.rgb *= texture(occlusmap, mapscale * fragment.position.xz).r;

	vec3 scatteredlight = ambient + lightcolor * diffuse;
	color.rgb = min(color.rgb * scatteredlight, vec3(1.0));

	float height = texture(heightmap, mapscale * fragment.position.xz).r;
	color.rgb = fog(color.rgb, length(viewspace), height);

	float gamma = 1.5;
	color.rgb = pow(color.rgb, vec3(1.0/gamma));
	/*


	color.rgb *= texture(occlusmap, mapscale * fragment.position.xz).r;

	vec3 normal = texture(normalmap, mapscale * fragment.position.xz).rgb;
	normal = (normal * 2.0) - 1.0;

	float diffuse = max(0.0, dot(normal, lightdirection));

	vec3 scatteredlight = ambient + lightcolor * diffuse;
	color.rgb = min(color.rgb * scatteredlight, vec3(1.0));

	color.rgb = fog(color.rgb, length(viewspace), 0.007 * fragment.position.y);

	float gamma = 1.6;
	color.rgb = pow(color.rgb, vec3(1.0/gamma));
	*/
}

