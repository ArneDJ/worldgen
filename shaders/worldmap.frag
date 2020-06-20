#version 430 core

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D colormap;
layout(binding = 2) uniform sampler2D normalmap;
layout(binding = 3) uniform sampler2DArray maskmaps;

layout(binding = 4) uniform sampler2D grassmap;
layout(binding = 5) uniform sampler2D alpinemap;
layout(binding = 6) uniform sampler2D desertmap;
layout(binding = 7) uniform sampler2D savannamap;
layout(binding = 8) uniform sampler2D badlandsmap;
layout(binding = 9) uniform sampler2D snowmap;

layout(binding = 10) uniform sampler2DArrayShadow shadowmap;

layout(binding = 11) uniform sampler2D rivermask;
layout(binding = 12) uniform sampler2D selectmask;

uniform float mapscale;
uniform vec3 camerapos;
uniform vec3 fogcolor;
uniform float fogfactor;

uniform vec4 split;
uniform mat4 shadowspace[4];

out vec4 color;

in TESSEVAL {
	vec3 position;
	vec2 texcoord;
	float zclipspace;
} fragment;

struct material {
	vec3 grass;
	vec3 dirt;
	vec3 stone;
	vec3 snow;
};

float random(in vec2 st) 
{
	return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise(in vec2 st) 
{
	vec2 i = floor(st);
	vec2 f = fract(st);

	// Four corners in 2D of a tile
	float a = random(i);
	float b = random(i + vec2(1.0, 0.0));
	float c = random(i + vec2(0.0, 1.0));
	float d = random(i + vec2(1.0, 1.0));

	vec2 u = f * f * (3.0 - 2.0 * f);

	return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

#define OCTAVES 5

float fbm(in vec2 st) 
{
	// Initial values
	float value = 0.0;
	float amplitude = .5;
	float frequency = 0.;
	//
	// Loop of octaves
	for (int i = 0; i < OCTAVES; i++) {
		value += amplitude * noise(st);
		st *= 2.;
		amplitude *= .5;
	}
	return value;
}

float shiftfbm(in vec2 _st) 
{
	float v = 0.0;
	float a = 0.5;
	vec2 shift = vec2(100.0);
	// Rotate to reduce axial bias
	mat2 rot = mat2(cos(0.5), sin(0.5),
		    -sin(0.5), cos(0.50));
	for (int i = 0; i < OCTAVES; ++i) {
		v += a * noise(_st);
		_st = rot * _st * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}

float warpfbm(vec2 st)
{
	vec2 q = vec2(0.);
	q.x = shiftfbm(st);
	q.y = shiftfbm(st + vec2(1.0));
	vec2 r = vec2(0.);
	r.x = shiftfbm( st + 20.0*q + vec2(1.7,9.2)+ 0.15);
	r.y = shiftfbm( st + 20.0*q + vec2(8.3,2.8)+ 0.126);

	return shiftfbm(st+r);
}

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
	float grassmask = texture(maskmaps, vec3(mapscale*fragment.texcoord, 0.0)).r;
	float alpmask = texture(maskmaps, vec3(mapscale*fragment.texcoord, 1.0)).r;
	float desertmask = texture(maskmaps, vec3(mapscale*fragment.texcoord, 2.0)).r;
	float savannamask = texture(maskmaps, vec3(mapscale*fragment.texcoord, 3.0)).r;
	float badlandsmask = texture(maskmaps, vec3(mapscale*fragment.texcoord, 4.0)).r;

	vec3 normal = texture(normalmap, mapscale*fragment.texcoord).rgb;
	float slope = 1.0 - normal.y;

	color = texture(grassmap, 0.05*fragment.texcoord);

	float height = texture(heightmap, mapscale*fragment.texcoord).r;

	color = mix(texture(desertmap, 0.05*fragment.texcoord), color, smoothstep(0.2, 0.22, height));
	color = mix(color, texture(snowmap, 0.05*fragment.texcoord), smoothstep(0.3, 0.4, height));

	color = mix(color, texture(alpinemap, 0.05*fragment.texcoord), alpmask*smoothstep(0.25, 0.4, slope));
	//color = mix(color, texture(alpinemap, 0.05*fragment.texcoord), alpmask);
	color = mix(color, texture(desertmap, 0.05*fragment.texcoord), desertmask);
	color = mix(color, texture(desertmap, 0.05*fragment.texcoord), badlandsmask);
	color = mix(color, texture(savannamap, 0.05*fragment.texcoord), savannamask);
	color = mix(color, texture(badlandsmap, 0.05*fragment.texcoord), 0.7*badlandsmask);

	vec3 lightdirection = vec3(0.5, 0.5, 0.5);
	float diffuse = max(0.0, dot(normal, lightdirection));

	vec4 purple = vec4(1.0, 0.0, 1.0, 1.0);
	float river = texture(rivermask, mapscale*fragment.texcoord).r;
	//color = mix(color, purple, river);
	float paintmask = texture(selectmask, mapscale*fragment.texcoord).r;
	color = mix(color, purple, 0.5*paintmask);
	color.rgb *= diffuse;
}
