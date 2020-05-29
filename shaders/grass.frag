#version 430 core

layout(binding = 1) uniform sampler2D normalmap;
layout(binding = 2) uniform sampler2D occlusmap;
layout(binding = 3) uniform sampler2D detailmap;
layout(binding = 4) uniform sampler2D colormap;

layout(binding = 10) uniform sampler2DArrayShadow shadowmap;

uniform float mapscale;
uniform vec3 camerapos;
uniform vec3 fogcolor;
uniform float fogfactor;
uniform vec4 split;
uniform mat4 shadowspace[4];

out vec4 color;

in GEOM {
	vec3 position;
	vec2 texcoord;
	float zclipspace;
} fragment;

vec3 fog(vec3 c, float dist, float height)
{
	float de = fogfactor * smoothstep(0.0, 3.3, 1.0 - height);
	float di = fogfactor * smoothstep(0.0, 5.5, 1.0 - height);
	float extinction = exp(-dist * de);
	float inscattering = exp(-dist * di);

	return c * extinction + fogcolor * (1.0 - inscattering);
}

float filterPCF(vec4 sc)
{
	ivec2 size = textureSize(shadowmap, 0).xy;
	float scale = 0.75;
	float dx = scale * 1.0 / float(size.x);
	float dy = scale * 1.0 / float(size.y);

	float shadow = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			vec4 coords = sc;
			coords.x += dx*x;
			coords.y += dy*y;
			shadow += texture(shadowmap, coords);
			count++;
		}
	}
	return shadow / count;
}

float shadow_coef(void)
{
	float shadow = 1.0;

	int cascade = 0;
	if (fragment.zclipspace < split.x) {
		cascade = 0;
	} else if (fragment.zclipspace < split.y) {
		cascade = 1;
	} else if (fragment.zclipspace < split.z) {
		cascade = 2;
	} else if (fragment.zclipspace < split.w) {
		cascade = 3;
	} else {
		return 1.0;
	}

	vec4 coords = shadowspace[int(cascade)] * vec4(fragment.position, 1.0);
	coords.w = coords.z;
	coords.z = float(cascade);
	shadow = filterPCF(coords);

	return clamp(shadow, 0.1, 1.0);
}

void main(void)
{
	const vec3 lightdirection = vec3(0.5, 0.5, 0.5);
	const vec3 ambient = vec3(0.5, 0.5, 0.5);
	const vec3 lightcolor = vec3(1.0, 1.0, 1.0);
	const vec3 viewspace = vec3(distance(fragment.position.x, camerapos.x), distance(fragment.position.y, camerapos.y), distance(fragment.position.z, camerapos.z));

	vec3 hue = vec3(0.34, 0.5, 0.09);
	color = texture(colormap, fragment.texcoord);
	if (color.a < 0.5) { discard; }
	color.a = 1.0;
	//vec3 color_bottom = 0.5 * color.rgb;
	color.rgb = mix(color.rgb, hue, 0.75);

	/*
	float dist = distance(camerapos, fragment.position);
	float blending = 1.0 / (0.01*dist);
	color.a *= blending*blending;
//	if (color.a < 0.5) { discard; }
	//if (color.a > 0.5) { color.a = 0.5; }
	*/

	color.rgb *= texture(occlusmap, mapscale * fragment.position.xz).r;

	vec3 normal = texture(normalmap, mapscale * fragment.position.xz).rgb;
	normal = (normal * 2.0) - 1.0;
	vec3 detail = texture(detailmap, fragment.position.xz*0.01).rgb;
	detail  = (detail * 2.0) - 1.0;
	detail = vec3(detail.x, detail.z, detail.y);

	normal = normalize((0.5 * detail) + normal);

	float diffuse = max(0.0, dot(normal, lightdirection));
	float shadow = 1.0;

	vec3 scatteredlight = ambient + lightcolor * diffuse * shadow;
	color.rgb = min(color.rgb * scatteredlight, vec3(1.0));

	color.rgb = fog(color.rgb, length(viewspace), 0.003 * fragment.position.y);

	float gamma = 1.5;
	color.rgb = pow(color.rgb, vec3(1.0/gamma));
	//color = vec4(1.0, 0.0, 0.0, 1.0);
}

