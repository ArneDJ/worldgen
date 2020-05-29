#version 430 core
#define M_PI 3.14159265358

layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 48) out;

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D normalmap;

uniform mat4 VIEW_PROJECT;
uniform float mapscale;
uniform float amplitude;
uniform float time;
uniform vec3 camerapos;

out GEOM {
	vec3 position;
	vec2 texcoord;
	float zclipspace;
} geom;

// individual grass blade
const vec3 v0 = vec3(0.0, 1.0, 0.0);
const vec3 v1 = vec3(-0.075, 0.25, 0.0);
const vec3 v2 = vec3(0.075, 0.25, 0.0);
const vec3 v3 = vec3(-0.075, 0.1, 0.0);
const vec3 v4 = vec3(0.1, 0.1, 0.0);
const vec3 v5 = vec3(-0.1, -1.0, 0.0);
const vec3 v6 = vec3(0.1, -1.0, 0.0);

float random(vec2 co)
{
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

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

mat3 rotationY(in float angle) 
{
	return mat3(
	cos(angle), 0, sin(angle),
	0, 1.0, 0,
	-sin(angle), 0, cos(angle));
}

mat3 AngleAxis3x3(float angle, vec3 axis)
{
	float s = sin(angle);
	float c = cos(angle);

	float t = 1 - c;
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;

	return mat3(
	t * x * x + c,      t * x * y - s * z,  t * x * z + s * y,
	t * x * y + s * z,  t * y * y + c,      t * y * z - s * x,
	t * x * z - s * y,  t * y * z + s * x,  t * z * z + c
	);
}

vec2 pos_to_uv(vec2 position)
{
	float x = 0.5 * (position.x + 1.0);
	float y = 0.5 * (position.y + 1.0);

	return vec2(x, 1.0 - y);
}

void make_vertex(vec3 position, vec2 uv)
{
	gl_Position = VIEW_PROJECT * vec4(position, 1.0);
	geom.texcoord = uv;
	geom.position = position;
	geom.zclipspace = gl_Position.z;
	EmitVertex();
}

/*
mat3 wind_rotation(vec3 origin)
{
	const float frequency = 0.02;
	float tiling = 0.003;
	vec2 uv = tiling*origin.xz + frequency * time;
	vec2 windsample = (texture(windmap, uv).rg * 2.0 - 1.0) * 2.0;
	vec3 wind = normalize(vec3(windsample.x, windsample.y, 0.0));
	mat3 R = AngleAxis3x3(windsample.x, wind) * AngleAxis3x3(windsample.y, wind);

	return R;
}
void make_grass_blade(vec3 origin, float stretch) 
{
	float len = noise(origin.xz);
	float scale = 0.5;

	mat3 R = rotationY(len*2.0*M_PI);
	mat3 wind = wind_rotation(origin);
	vec3 normal = texture(normalmap, mapscale*origin.xz).rgb;
	mat3 spin = AngleAxis3x3(len, -normal);
	R = spin * R;

	vec3 a =  wind * R * vec3(stretch * scale * v0.x, scale * v0.y, stretch * scale * v0.z);
	vec3 b =  wind * R * vec3(stretch * scale * v1.x, scale * v1.y, stretch * scale * v1.z);
	vec3 c =  wind * R * vec3(stretch * scale * v2.x, scale * v2.y, stretch * scale * v2.z);
	vec3 d =  wind * R * vec3(stretch * scale * v3.x, scale * v3.y, stretch * scale * v3.z);
	vec3 e =  wind * R * vec3(stretch * scale * v4.x, scale * v4.y, stretch * scale * v4.z);
	vec3 f =  R * vec3(stretch * scale * v5.x, scale * v5.y, stretch * scale * v5.z);
	vec3 g =  R * vec3(stretch * scale * v6.x, scale * v6.y, stretch * scale * v6.z);

	make_vertex(origin+a, v0.xy);
	make_vertex(origin+b, v1.xy);
	make_vertex(origin+c, v2.xy);
	make_vertex(origin+d, v3.xy);
	make_vertex(origin+e, v4.xy);
	make_vertex(origin+f, v5.xy);
	make_vertex(origin+g, v6.xy);

	EndPrimitive();
}
*/

const vec3 a = vec3(-1.0, 1.0, 0.0);
const vec3 b = vec3(1.0, 1.0, 0.0);
const vec3 c = vec3(-1.0, -0.25, 0.0);
const vec3 d = vec3(1.0, -0.25, 0.0);

const vec3 e = vec3(-0.7, 1.0, -0.7);
const vec3 f = vec3(0.7, 1.0, 0.7);
const vec3 g = vec3(-0.7, -0.25, -0.7);
const vec3 h = vec3(0.7, -0.25, 0.7);

const vec3 i = vec3(-0.7, 1.0, 0.7);
const vec3 j = vec3(0.7, 1.0, -0.7);
const vec3 k = vec3(-0.7, -0.25, 0.7);
const vec3 l = vec3(0.7, -0.25, -0.7);

void make_cardinal_quads(vec3 origin)
{
	float len = noise(origin.xz);
	vec3 normal = texture(normalmap, mapscale*origin.xz).rgb;
	mat3 spin = AngleAxis3x3(len, -normal);

	make_vertex(origin+spin*a, vec2(0.0, 0.0));
	make_vertex(origin+spin*b, vec2(1.0, 0.0));
	make_vertex(origin+spin*c, vec2(0.0, 1.0));
	make_vertex(origin+spin*d, vec2(1.0, 1.0));
	EndPrimitive();
	make_vertex(origin+spin*e, vec2(0.0, 0.0));
	make_vertex(origin+spin*f, vec2(1.0, 0.0));
	make_vertex(origin+spin*g, vec2(0.0, 1.0));
	make_vertex(origin+spin*h, vec2(1.0, 1.0));
	EndPrimitive();
	make_vertex(origin+spin*i, vec2(0.0, 0.0));
	make_vertex(origin+spin*j, vec2(1.0, 0.0));
	make_vertex(origin+spin*k, vec2(0.0, 1.0));
	make_vertex(origin+spin*l, vec2(1.0, 1.0));
	EndPrimitive();

}

void main(void)
{
	vec3 position = gl_in[0].gl_Position.xyz;
	position.y = amplitude * texture(heightmap, mapscale*position.xz).r;
	float dist = distance(camerapos, position);

	if (dist < 32.0) {
		make_cardinal_quads(position);
		//if (dist < 16.0) {
		//make_cardinal_quads(position+vec3(0.5, 0.0, 0.0));
		//make_cardinal_quads(position+vec3(0.0, 0.0, 0.5));
		//}
	}
}
