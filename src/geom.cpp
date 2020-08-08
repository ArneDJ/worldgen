#include <algorithm>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "geom.h"

#define SMALL_NUM 0.00000001f
#define perp(u,v) ((u).x * (v).y - (u).y * (v).x)  // perp product  (2D)

static void swap(float *a, float *b)
{
	float tmp = *b;
	*b = *a;
	*a = tmp;
}

bool point_in_rectangle(glm::vec2 p, struct rectangle r)
{
	return p.x >= r.min.x && p.x < r.max.x && p.y >= r.min.y && p.y < r.max.y;
}

bool point_in_circle(glm::vec2 p, struct circle c)
{
	return ((p.x - c.center.x)*(p.x - c.center.x) + (p.y - c.center.y)*(p.y - c.center.y)) < c.radius*c.radius;
}

// Test if point P lies inside the counterclockwise triangle ABC
bool point_in_triangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	// Translate point and triangle so that point lies at origin
	a -= p; b -= p; c -= p;
	// Compute normal vectors for triangles pab and pbc
	glm::vec3 u = glm::cross(b, c);
	glm::vec3 v = glm::cross(c, a);
	// Make sure they are both pointing in the same direction
	if (glm::dot(u, v) < 0.0f) { return false; }
	// Compute normal vector for triangle pca
	glm::vec3 w = glm::cross(a, b);
	// Make sure it points in the same direction as the first two
	if (glm::dot(u, w) < 0.0f) { return false; }
	// Otherwise P must be in (or on) the triangle
	return true;
}

// found this on wikipedia, seems to work
bool ray_in_triangle(glm::vec3 origin, glm::vec3 dir, const struct triangle *tri)
{
	const float EPSILON = 0.0000001f;
	glm::vec3 vertex0 = tri->a;
	glm::vec3 vertex1 = tri->b;
	glm::vec3 vertex2 = tri->c;
	glm::vec3 edge1, edge2, h, s, q;
	float a,f,u,v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	h = glm::cross(dir, edge2);
	a = glm::dot(edge1, h);

	if (a > -EPSILON && a < EPSILON) { return false; }   // This ray is parallel to this triangle.

	f = 1.0/a;
	s = origin - vertex0;
	u = f * glm::dot(s, h);

	if (u < 0.0 || u > 1.0) { return false; }

	q = glm::cross(s, edge1);
	v = f * glm::dot(dir, q);

	if (v < 0.0 || u + v > 1.0) { return false; }

	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * glm::dot(edge2, q);
	if (t > EPSILON && t < 1/EPSILON) {
		//vec3 tmp = vec3_sum(origin, vec3_scale(t, dir));
		//out->x = tmp.x; out->y = tmp.y; out->z = tmp.z;
		//dist = &t;
		return true;
	} else { // This means that there is a line intersection but not a ray intersection.
		return false;
	}
}

struct ray_triangle_intersection intersect_ray_in_triangle(glm::vec3 origin, glm::vec3 dir, const struct triangle *tri)
{
	struct ray_triangle_intersection intersect = {
		.intersects = false,
		.distance = 0.f,
		.point = glm::vec3(0.f),
	};

	const float EPSILON = 0.0000001f;
	glm::vec3 vertex0 = tri->a;
	glm::vec3 vertex1 = tri->b;
	glm::vec3 vertex2 = tri->c;
	glm::vec3 edge1, edge2, h, s, q;
	float a,f,u,v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	h = glm::cross(dir, edge2);
	a = glm::dot(edge1, h);

	if (a > -EPSILON && a < EPSILON) { 
		return intersect; 
	}   // This ray is parallel to this triangle.

	f = 1.0/a;
	s = origin - vertex0;
	u = f * glm::dot(s, h);

	if (u < 0.0 || u > 1.0) { return intersect; }

	q = glm::cross(s, edge1);
	v = f * glm::dot(dir, q);

	if (v < 0.0 || u + v > 1.0) { return intersect; }

	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * glm::dot(edge2, q);
	if (t > EPSILON && t < 1/EPSILON) {
		intersect.intersects = true;
		intersect.point = origin + (t * dir);
		intersect.distance = t;
		return intersect;
	} else { // This means that there is a line intersection but not a ray intersection.
		return intersect;
	}
}

bool ray_in_AABB(glm::vec3 p, glm::vec3 d, struct AABB box)
{
	using namespace glm;

	vec3 min = {box.c.x - box.r.x, box.c.y - box.r.y, box.c.z - box.r.z};
	vec3 max = {box.c.x + box.r.x, box.c.y + box.r.y, box.c.z + box.r.z};

	float tmin = (min.x - p.x) / d.x;
	float tmax = (max.x - p.x) / d.x;

	if (tmin > tmax) { swap(&tmin, &tmax); }

	float tymin = (min.y - p.y) / d.y;
	float tymax = (max.y - p.y) / d.y;

	if (tymin > tymax) { swap(&tymin, &tymax); }

	if ((tmin > tymax) || (tymin > tmax)) { return false; }

	if (tymin > tmin) { tmin = tymin; }

	if (tymax < tmax) { tmax = tymax; }

	float tzmin = (min.z - p.z) / d.z;
	float tzmax = (max.z - p.z) / d.z;

	if (tzmin > tzmax) { swap(&tzmin, &tzmax); }

	if ((tmin > tzmax) || (tzmin > tmax)) { return false; }

	if (tzmin > tmin) { tmin = tzmin; }

	if (tzmax < tmax) { tmax = tzmax; }

	return true;
}

struct ray_AABB_intersection intersect_ray_in_AABB(glm::vec3 p, glm::vec3 d, struct AABB box)
{
	struct ray_AABB_intersection intersect = {
		.intersects = false,
		.tmin = 0.f,
		.tmax = 0.f,
	};

	using namespace glm;

	vec3 min = {box.c.x - box.r.x, box.c.y - box.r.y, box.c.z - box.r.z};
	vec3 max = {box.c.x + box.r.x, box.c.y + box.r.y, box.c.z + box.r.z};

	intersect.tmin = (min.x - p.x) / d.x;
	intersect.tmax = (max.x - p.x) / d.x;

	if (intersect.tmin > intersect.tmax) { swap(&intersect.tmin, &intersect.tmax); }

	float tymin = (min.y - p.y) / d.y;
	float tymax = (max.y - p.y) / d.y;

	if (tymin > tymax) { swap(&tymin, &tymax); }

	if ((intersect.tmin > tymax) || (tymin > intersect.tmax)) { 
		intersect.intersects = false; 
		return intersect;
	}

	if (tymin > intersect.tmin) { intersect.tmin = tymin; }

	if (tymax < intersect.tmax) { intersect.tmax = tymax; }

	float tzmin = (min.z - p.z) / d.z;
	float tzmax = (max.z - p.z) / d.z;

	if (tzmin > tzmax) { swap(&tzmin, &tzmax); }

	if ((intersect.tmin > tzmax) || (tzmin > intersect.tmax)) { 
		intersect.intersects = false; 
		return intersect;
	} else {
		intersect.intersects = true;
	}

	if (tzmin > intersect.tmin) { intersect.tmin = tzmin; }

	if (tzmax < intersect.tmax) { intersect.tmax = tzmax; }

	return intersect;
}

glm::vec3 screen_to_ray(float x, float y, float width, float height, glm::mat4 view, glm::mat4 project)
{
	glm::vec4 clip = glm::vec4((2.f * x) / width - 1.f, 1.f - (2.f * y) / height, -1.f, 1.f);

	glm::vec4 worldspace = glm::inverse(project) * clip;
	glm::vec4 eye = glm::vec4(worldspace.x, worldspace.y, -1.f, 0.f);
	glm::vec3 ray = glm::vec3(glm::inverse(view) * eye);

	return glm::normalize(ray);
}

glm::vec2 screen_to_ndc(float x, float y, float width, float height) 
{
	float nx = (2.f * x) / width - 1.f;
	float ny = 1.f - (2.f * y) / height;

	return glm::vec2(nx, ny);
}

glm::vec2 ndc_to_view(float nx, float ny, float width, float height) 
{
	float x = 0.5f * (width * (nx + 1.f));
	float y = height * (0.5f *(ny + 1.f));

	return glm::vec2(x, y);
}

static float sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
{
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool point_in_triangle(glm::vec2 pt, struct triangle tri)
{
	float d1, d2, d3;
	bool has_neg, has_pos;

	d1 = sign(pt, glm::vec2(tri.a.x, tri.a.y), glm::vec2(tri.b.x, tri.b.y));
	d2 = sign(pt, glm::vec2(tri.b.x, tri.b.y), glm::vec2(tri.c.x, tri.c.y));
	d3 = sign(pt, glm::vec2(tri.c.x, tri.c.y), glm::vec2(tri.a.x, tri.a.y));

	has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
	has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

	return !(has_neg && has_pos);
}

bool in_segment(glm::vec2 P, struct segment S)
{
	if (S.P0.x != S.P1.x) {    // S is not  vertical
		if (S.P0.x <= P.x && P.x <= S.P1.x) { return true; }
		if (S.P0.x >= P.x && P.x >= S.P1.x) { return true; }
	} else {    // S is vertical, so test y  coordinate
		if (S.P0.y <= P.y && P.y <= S.P1.y) { return true; }
		if (S.P0.y >= P.y && P.y >= S.P1.y) { return true; }
	}

	return false;
}

bool segment_intersects_segment(struct segment S1, struct segment S2)
{
	using namespace glm;
	const vec2 a0 = S1.P0;
	const vec2 a1 = S1.P1;
	const vec2 b0 = S2.P0;
	const vec2 b1 = S2.P1;

	vec2 u = {a1.x - a0.x, a1.y - a0.y};
	vec2 v = {b1.x - b0.x, b1.y - b0.y};
	vec2 w = {a0.x - b0.x, a0.y - b0.y};
	float D = perp(u,v);

	if (fabs(D) < SMALL_NUM) {           // S1 and S2 are parallel
		if (perp(u,w) != 0.f || perp(v,w) != 0.f)  { 
			return false; 
		} // they are NOT collinear

		// they are collinear or degenerate
		// check if they are degenerate  points
		float du = dot(u,u);
		float dv = dot(v,v);

		if (du == 0.f && dv == 0.f) {            // both segments are points
			if (a0.x !=  b0.x  && a0.y != b0.y) { 
				return false; 
			} // they are distinct  points
			return true;
		}
		if (du == 0) {                     // S1 is a single point
			if  (in_segment(S1.P0, S2) == 0) { 
				return false; 
			} // but is not in S2
			return true;
		}
		if (dv == 0) {                     // S2 a single point
			if  (in_segment(S2.P0, S1) == 0) { 
				return false; 
			} // but is not in S1
			return true;
		}

		float t0, t1;                    // endpoints of S1 in eqn for S2
		vec2 w2 = {S1.P1.x - S2.P0.x, S1.P1.y - S2.P0.y};
		if (v.x != 0) {
			t0 = w.x / v.x;
			t1 = w2.x / v.x;
		} else {
			t0 = w.y / v.y;
			t1 = w2.y / v.y;
		}
		if (t0 > t1) {                   // must have t0 smaller than t1
			float t = t0; t0 = t1; t1 = t;    // swap if not
		}
		if (t0 > 1.f || t1 < 0.f) { return false; } // NO overlap
		t0 = t0 < 0.f ? 0.f : t0; // clip to min 0
		t1 = t1 > 1.f ? 1.f : t1; // clip to max 1
		if (t0 == t1) { return true; } // intersect is a point

		// they overlap in a valid subsegment
		return true;
	 }

	// the segments are skew and may intersect in a point
	// get the intersect parameter for S1
	float  si = perp(v,w) / D;
	if (si < 0.f || si > 1.f) { return false; } // no intersect with S1

	// get the intersect parameter for S2
	float ti = perp(u,w) / D;
	if (ti < 0.f || ti > 1.f) { return false; } // no intersect with S2

	return true;
}

bool segment_overlaps_triangle(struct segment s, struct triangle tri)
{
	struct segment ab = { tri.a, tri.b };
	struct segment bc = { tri.b, tri.c };
	struct segment ca = { tri.c, tri.a };

	if (point_in_triangle(s.P0, tri)) {
		return true;
	} else if (point_in_triangle(s.P1, tri)) {
		return true;
	}

	if (segment_intersects_segment(s, ab)) {
		return true;
	} else if (segment_intersects_segment(s, bc)) {
		return true;
	} else if (segment_intersects_segment(s, ca)) {
		return true;
	}

	return false;
}

bool segment_overlaps_rectangle(struct segment s, struct rectangle rect)
{
	struct segment S0 = { rect.min, {rect.min.x, rect.max.y} };
	struct segment S1 = { {rect.min.x, rect.max.y}, rect.max };
	struct segment S2 = { rect.max, {rect.max.x, rect.min.y} };
	struct segment S3 = { {rect.max.x, rect.min.y}, rect.min };

	if (point_in_rectangle(s.P0, rect)) {
		return true;
	} else if (point_in_rectangle(s.P1, rect)) {
		return true;
	}

	if (segment_intersects_segment(s, S0)) {
		return true;
	} else if (segment_intersects_segment(s, S1)) {
		return true;
	} else if (segment_intersects_segment(s, S2)) {
		return true;
	} else if (segment_intersects_segment(s, S3)) {
		return true;
	}

	return false;
}

bool rectangle_inside_triangle(struct rectangle rect, struct triangle tri)
{
	if (point_in_triangle(rect.min, tri)) {
		return true;
	}
	if (point_in_triangle(rect.max, tri)) {
		return true;
	}

	return false;
}

bool triangle_overlaps_rectangle(struct triangle tri, struct rectangle rect)
{
	// check if any of the triangle points are in rectangle
	if (point_in_rectangle(tri.a, rect)) {
		return true;
	} else if (point_in_rectangle(tri.b, rect)) {
		return true;
	} else if (point_in_rectangle(tri.c, rect)) {
		return true;
	}

	// the triangle can also be so big that the rectangle is inside of it
	if (rectangle_inside_triangle(rect, tri)) {
		return true;
	}

	// none of the points are in the rectangle but the sides can still intersect
	struct segment S0 = { rect.min, {rect.min.x, rect.max.y} };
	struct segment S1 = { {rect.min.x, rect.max.y}, rect.max };
	struct segment S2 = { rect.max, {rect.max.x, rect.min.y} };
	struct segment S3 = { {rect.max.x, rect.min.y}, rect.min };

	if (segment_overlaps_triangle(S0, tri)) {
		return true;
	} else if (segment_overlaps_triangle(S1, tri)) {
		return true;
	} else if (segment_overlaps_triangle(S2, tri)) {
		return true;
	} else if (segment_overlaps_triangle(S3, tri)) {
		return true;
	}
	
	return false;
}

glm::vec3 barycentric(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 p)
{
	using namespace glm;

	vec3 v0 = b - a;
	vec3 v1 = c - a;
	vec3 v2 = p - a;

	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);

	float denom = d00 * d11 - d01 * d01;

	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0 - v - w;

	vec3 tmp = {u, v, w};
	return tmp;
}

struct plane compute_plane(glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	struct plane p;
	p.n = glm::normalize(glm::cross(b - a, c - a));
	p.d = glm::dot(p.n, a);

	return p;
}

static float y_in_plane(glm::vec2 p, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) 
{
	float a = -(p3.z*p2.y-p1.z*p2.y-p3.z*p1.y+p1.y*p2.z+p3.y*p1.z-p2.z*p3.y);
	float b = (p1.z*p3.x+p2.z*p1.x+p3.z*p2.x-p2.z*p3.x-p1.z*p2.x-p3.z*p1.x);
	float c = (p2.y*p3.x+p1.y*p2.x+p3.y*p1.x-p1.y*p3.x-p2.y*p1.x-p2.x*p3.y);
	float d = -a*p1.x-b*p1.y-c*p1.z;

	return -(a*p.x+c*p.y+d)/b;
}

glm::vec3 point_on_triangle(glm::vec2 point, struct triangle tri)
{
	glm::vec3 q = {point.x, 0.f, point.y};

	q.y = y_in_plane(point, tri.a, tri.b, tri.c);

	return q;
}

bool point_in_circle(glm::vec2 point, glm::vec2 origin, float radius)
{
	return ((point.x - origin.x)*(point.x - origin.x)) + ((point.y - origin.y)*(point.y - origin.y)) <= (radius*radius);
}

bool AABB_in_frustum(glm::vec3 &Min, glm::vec3 &Max, glm::vec4 frustum_planes[6]) 
{ 
	bool inside = true; //test all 6 frustum planes 
	for (int i = 0; i < 6; i++) { //pick closest point to plane and check if it behind the plane //if yes - object outside frustum 
		float d = std::max(Min.x * frustum_planes[i].x, Max.x * frustum_planes[i].x) + std::max(Min.y * frustum_planes[i].y, Max.y * frustum_planes[i].y) + std::max(Min.z * frustum_planes[i].z, Max.z * frustum_planes[i].z) + frustum_planes[i].w; 
		inside &= d > 0; //return false; //with flag works faster 
	} 

	return inside; 
}

void frustum_to_planes(glm::mat4 M, glm::vec4 planes[6])
{
	planes[0] = {
		M[0][3] + M[0][0],
		M[1][3] + M[1][0],
		M[2][3] + M[2][0],
		M[3][3] + M[3][0],
	};
	planes[1] = {
		M[0][3] - M[0][0],
		M[1][3] - M[1][0],
		M[2][3] - M[2][0],
		M[3][3] - M[3][0],
	};
	planes[2] = {
		M[0][3] + M[0][1],
		M[1][3] + M[1][1],
		M[2][3] + M[2][1],
		M[3][3] + M[3][1],
	};
	planes[3] = {
		M[0][3] - M[0][1],
		M[1][3] - M[1][1],
		M[2][3] - M[2][1],
		M[3][3] - M[3][1],
	};
	planes[4] = {
		M[0][2],
		M[1][2],
		M[2][2],
		M[3][2],
	};
	planes[5] = {
		M[0][3] - M[0][2],
		M[1][3] - M[1][2],
		M[2][3] - M[2][2],
		M[3][3] - M[3][2],
	};
}

glm::vec2 segment_midpoint(glm::vec2 a, glm::vec2 b)
{
	return glm::vec2((a.x+b.x)/2.f, (a.y+b.y)/2.f);
}

bool clockwise(glm::vec2 a, glm::vec2 b, glm::vec2 c) 
{
	int wise = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);

	if (wise < 0) {
		return true;
	} else {
		return false;
	}
}
