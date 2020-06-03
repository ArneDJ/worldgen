#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <set>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#define JC_VORONOI_IMPLEMENTATION
#include "../external/voronoi/jc_voronoi.h"

#include "imp.h"
#include "voronoi.h"

// Remaps the point from the input space to image space
static inline jcv_point remap(const jcv_point* pt, const jcv_point* min, const jcv_point* max, const jcv_point* scale)
{
	jcv_point p;
	p.x = (pt->x - min->x)/(max->x - min->x) * scale->x;
	p.y = (pt->y - min->y)/(max->y - min->y) * scale->y;
	return p;
}

static void relax_points(const jcv_diagram* diagram, std::vector<jcv_point> &points)
{
	const jcv_site* sites = jcv_diagram_get_sites(diagram);
	for( int i = 0; i < diagram->numsites; ++i ) {
		const jcv_site* site = &sites[i];
		jcv_point sum = site->p;
		int count = 1;

		const jcv_graphedge* edge = site->edges;

		while (edge) {
			sum.x += edge->pos[0].x;
			sum.y += edge->pos[0].y;
			++count;
			edge = edge->next;
		}

		jcv_point point;
		point.x = sum.x / count;
		point.y = sum.y / count;
		points.push_back(point);
	}
}

struct corner {
	int index;
	glm::vec2 position;
	std::vector<struct corner*> adjacent;
};

struct border {
	struct mycorner *a;
	struct mycorner *b;
};

struct cell {
	glm::vec2 center;
	std::vector<glm::vec4> borders;
	std::vector<struct cell*> neighbors;
	// std::vector<struct corner*> corners;
};

void gen_diagram(struct byteimage *image, std::vector<glm::vec2> &locations)
{
	std::vector<jcv_point> points;

	for (auto &location : locations) {
		jcv_point point;
		point.x = location.x;
		point.y = location.y;
		points.push_back(point);
	}

	jcv_point dimensions;
	dimensions.x = (jcv_real)image->width;
	dimensions.y = (jcv_real)image->height;
	const jcv_rect rect = {
		{0.0, 0.0},
		dimensions,
	};
	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(points.size(), points.data(), &rect, &diagram);

	std::vector<jcv_point> relaxed_points;
	relax_points(&diagram, relaxed_points);

	jcv_diagram_generate(relaxed_points.size(), relaxed_points.data(), &rect, &diagram);

	const jcv_site *sites = jcv_diagram_get_sites(&diagram);

	struct cell *cells = new struct cell[diagram.numsites];
	// get each cell
	for (int i = 0; i < diagram.numsites; i++) {
		const jcv_site *site = &sites[i];
		struct cell cell;
		jcv_point p = site->p;
		cell.center = glm::vec2(p.x, p.y);
		const jcv_graphedge *edge = site->edges;
		while (edge) {
			cell.borders.push_back(glm::vec4(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y));
			edge = edge->next;
		}
		cells[site->index] = cell;
	}

	// get neighbor cells
	for (int i = 0; i < diagram.numsites; i++) { 
		const jcv_site *site = &sites[i];
		const jcv_graphedge *edge = site->edges;
		while (edge) {
			const jcv_site *neighbor = edge->neighbor;
			if (neighbor != nullptr) {
				cells[site->index].neighbors.push_back(&cells[neighbor->index]);
			}

			edge = edge->next;
		}
	}

	// generate the vertices and vertex_edges
	jcv_diagram_generate_vertices(&diagram);

	struct corner *mycorners = new struct corner[diagram.internal->numvertices];

	const jcv_vertex *vertex = jcv_diagram_get_vertices(&diagram);
	while (vertex) {
		struct corner c;
		c.index = vertex->index;
		c.position = glm::vec2(vertex->pos.x, vertex->pos.y);
		jcv_vertex_edge *edges = vertex->edges; // The half edges owned by the vertex
		while (edges) {
			jcv_vertex *neighbor = edges->neighbor;
			edges = edges->next;
			c.adjacent.push_back(&mycorners[neighbor->index]);
		}
		mycorners[vertex->index] = c;
		vertex = jcv_diagram_get_next_vertex(vertex);
	}

	unsigned char sitecolor[3] = {255, 255, 255};
	unsigned char linecolor[3] = {255, 255, 255};
	unsigned char delacolor[3] = {255, 0, 0};
	unsigned char blue[3] = {0, 0, 255};
	unsigned char white[3] = {255, 255, 255};

	cell cell = cells[28];
	plot(int(cell.center.x), int(cell.center.y), image->data, image->width, image->height, image->nchannels, sitecolor);
	for (auto &neighbor : cell.neighbors) {
		unsigned char rcolor[3];
		unsigned char basecolor = 100;
		rcolor[0] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[1] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[2] = basecolor + (unsigned char)(rand() % (235 - basecolor));

		for (auto &border : neighbor->borders) {
			draw_triangle(neighbor->center.x, neighbor->center.y, border.x, border.y, border.z, border.w, image->data, image->width, image->height, image->nchannels, rcolor);
		}
	}

	struct corner c = mycorners[40];
	for (auto &neighbor : c.adjacent) {
		draw_line(c.position.x, c.position.y, neighbor->position.x, neighbor->position.y, image->data, image->width, image->height, image->nchannels, white);
		plot((int)neighbor->position.x, (int)neighbor->position.y, image->data, image->width, image->height, image->nchannels, blue);
	}
	plot((int)c.position.x, (int)c.position.y, image->data, image->width, image->height, image->nchannels, blue);

	delete [] cells;
	delete [] mycorners;
	jcv_diagram_free(&diagram);
}
