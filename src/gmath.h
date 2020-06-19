struct triangle {
	glm::vec3 a, b, c;
};
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

/* COUNTER CLOCKWISE */
/*
int ray_in_triangle(glm::vec3 p, glm::vec3 q, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	glm::vec3 bc = b - c;
	glm::vec3 ac = a - c;
	glm::vec3 qp = p - q;

	glm::vec3 n = glm::cross(bc, ac);

 float d = glm::dot(qp, n);
 if (d <= 0.0)
  return 0;

 glm::vec3 ap = p - c;
 float t = glm::dot(ap, n);
 if (t < 0.0)
  return 0;

 glm::vec3 e = glm::cross(qp, ap);
 float v = glm::dot(ac, e);
 if (v < 0.0 || v > d)
  return 0;

 float w = -glm::dot(bc, e);
 if (w < 0.0 || v + w > d)
  return 0;

 return 1;
}
*/

bool ray_intersects_triangle(glm::vec3 rayOrigin, glm::vec3 rayVector, const struct triangle *tri)
{
 const float EPSILON = 0.0000001;
 glm::vec3 vertex0 = tri->a;
 glm::vec3 vertex1 = tri->b;
 glm::vec3 vertex2 = tri->c;
 glm::vec3 edge1, edge2, h, s, q;
 float a,f,u,v;
 edge1 = vertex1 - vertex0;
 edge2 = vertex2 - vertex0;
 h = glm::cross(rayVector, edge2);
 a = glm::dot(edge1, h);

 if (a > -EPSILON && a < EPSILON)
  return 0;    // This ray is parallel to this triangle.

 f = 1.0/a;
 s = rayOrigin - vertex0;
 u = f * glm::dot(s, h);

 if (u < 0.0 || u > 1.0)
  return 0;

 q = glm::cross(s, edge1);
 v = f * glm::dot(rayVector, q);

 if (v < 0.0 || u + v > 1.0)
  return 0;

 // At this stage we can compute t to find out where the intersection point is on the line.
 float t = f * glm::dot(edge2, q);
 if (t > EPSILON && t < 1/EPSILON) {
  //vec3 tmp = vec3_sum(rayOrigin, vec3_scale(t, rayVector));
  //out->x = tmp.x; out->y = tmp.y; out->z = tmp.z;
  //dist = &t;
  return 1;
 } else // This means that there is a line intersection but not a ray intersection.
  return 0;
}


bool ray_in_triangle(glm::vec3 orig, glm::vec3 dir, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2)
{
    // compute plane's normal
	glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v0v2 = v2 - v0;
    // no need to normalize
    glm::vec3 N = glm::cross(v0v1,v0v2); // N
    float area2 = glm::length(N);

    // Step 1: finding P

    // check if ray and plane are parallel ?
    float NdotRayDirection = glm::dot(N, dir);
    if (fabs(NdotRayDirection) < FLT_EPSILON) // almost 0
        return false; // they are parallel so they don't intersect !

    // compute d parameter using equation 2
    float d = glm::dot(N, v0);

    // compute t (equation 3)
    float t = (glm::dot(N, orig) + d) / NdotRayDirection;
    // check if the triangle is in behind the ray
    if (t < 0) return false; // the triangle is behind

    // compute the intersection point using equation 1
    glm::vec3 P = orig + t * dir;

    // Step 2: inside-outside test
    glm::vec3 C; // vector perpendicular to triangle's plane

    // edge 0
    glm::vec3 edge0 = v1 - v0;
    glm::vec3 vp0 = P - v0;
    C = glm::cross(edge0, vp0);
    if (glm::dot(N, C) < 0.f) return false; // P is on the right side

    // edge 1
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 vp1 = P - v1;
    C = glm::cross(edge1, vp1);
    if (glm::dot(N, C) < 0.f)  return false; // P is on the right side

    // edge 2
    glm::vec3 edge2 = v0 - v2;
    glm::vec3 vp2 = P - v2;
    C = glm::cross(edge2, vp2);
    if (glm::dot(N, C) < 0.f) return false; // P is on the right side;

    return true; // this ray hits the triangle
}
