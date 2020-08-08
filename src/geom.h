struct segment {
	glm::vec2 P0, P1;
};

struct rectangle {
	glm::vec2 min;
	glm::vec2 max;
};

struct triangle {
	glm::vec3 a, b, c;
};

struct plane {
	glm::vec3 n;
	float d;
};

struct circle {
	glm::vec2 center;
	float radius;
};

struct AABB {
	glm::vec3 c; // center point of AABB
	glm::vec3 r; // radius or halfwidth extents (rx, ry, rz)
};

struct ray_AABB_intersection {
	bool intersects;
	float tmin;
	float tmax;
};

struct ray_triangle_intersection {
	bool intersects;
	float distance;
	glm::vec3 point; // intersection point
};

bool point_in_rectangle(glm::vec2 p, struct rectangle r);

bool point_in_circle(glm::vec2 p, struct circle c);

glm::vec3 screen_to_ray(float x, float y, float width, float height, glm::mat4 view, glm::mat4 project);

// screen space coordinates to normalized device coordinates between -1 and 1
glm::vec2 screen_to_ndc(float x, float y, float width, float height);

glm::vec2 ndc_to_view(float nx, float ny, float width, float height);

bool ray_in_AABB(glm::vec3 p, glm::vec3 d, struct AABB box);

struct ray_AABB_intersection intersect_ray_in_AABB(glm::vec3 p, glm::vec3 d, struct AABB box);

bool ray_in_triangle(glm::vec3 origin, glm::vec3 dir, const struct triangle *tri);

struct ray_triangle_intersection intersect_ray_in_triangle(glm::vec3 origin, glm::vec3 dir, const struct triangle *tri);

bool point_in_triangle(glm::vec2 pt, struct triangle tri);

bool triangle_overlaps_rectangle(struct triangle tri, struct rectangle rect);

bool segment_overlaps_triangle(struct segment s, struct triangle tri);

bool segment_overlaps_rectangle(struct segment s, struct rectangle rect);

glm::vec3 point_on_triangle(glm::vec2 point, struct triangle tri);

bool point_in_circle(glm::vec2 point, glm::vec2 origin, float radius);

bool segment_intersects_segment(struct segment S1, struct segment S2);

bool AABB_in_frustum(glm::vec3 &Min, glm::vec3 &Max, glm::vec4 frustum_planes[6]);

void frustum_to_planes(glm::mat4 M, glm::vec4 planes[6]);

glm::vec2 segment_midpoint(glm::vec2 a, glm::vec2 b);

bool clockwise(glm::vec2 a, glm::vec2 b, glm::vec2 c);
