struct triangle {
	glm::vec3 a, b, c;
};

glm::vec2 midpoint(glm::vec2 a, glm::vec2 b)
{
	return glm::vec2((a.x+b.x)/2.f, (a.y+b.y)/2.f);
}

//
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

bool ray_in_triangle(glm::vec3 rayOrigin, glm::vec3 rayVector, const struct triangle *tri)
{
	const float EPSILON = 0.0000001f;
	glm::vec3 vertex0 = tri->a;
	glm::vec3 vertex1 = tri->b;
	glm::vec3 vertex2 = tri->c;
	glm::vec3 edge1, edge2, h, s, q;
	float a,f,u,v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	h = glm::cross(rayVector, edge2);
	a = glm::dot(edge1, h);

	if (a > -EPSILON && a < EPSILON) { return 0; }   // This ray is parallel to this triangle.

	f = 1.0/a;
	s = rayOrigin - vertex0;
	u = f * glm::dot(s, h);

	if (u < 0.0 || u > 1.0) { return 0; }

	q = glm::cross(s, edge1);
	v = f * glm::dot(rayVector, q);

	if (v < 0.0 || u + v > 1.0) { return 0; }

	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * glm::dot(edge2, q);
	if (t > EPSILON && t < 1/EPSILON) {
		//vec3 tmp = vec3_sum(rayOrigin, vec3_scale(t, rayVector));
		//out->x = tmp.x; out->y = tmp.y; out->z = tmp.z;
		//dist = &t;
		return 1;
	} else { // This means that there is a line intersection but not a ray intersection.
		return 0;
	}
}

glm::vec3 screenpos_to_ray(int x, int y, float windowwidth, float windowheight, glm::mat4 view, glm::mat4 project)
{
	glm::vec4 clip = glm::vec4((2.f * float(x)) / windowwidth - 1.f, 1.f - (2.f * float(y)) / windowheight, -1.f, 1.f);

	glm::vec4 worldspace = glm::inverse(project) * clip;
	glm::vec4 ray_eye = glm::vec4(worldspace.x, worldspace.y, -1.f, 0.f);
	glm::vec3 ray_world = glm::vec3(glm::inverse(view) * ray_eye);

	return glm::normalize(ray_world);
}

