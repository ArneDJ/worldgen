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

static void prune_corners(std::vector<const struct corner*> &v)
{
	auto end = v.end();
	for (auto it = v.begin(); it != end; ++it) {
		end = std::remove(it + 1, end, *it);
	}

	v.erase(end, v.end());
}

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

void Voronoi::gen_diagram(std::vector<glm::vec2> &locations, size_t width, size_t height)
{
	std::vector<jcv_point> points;

	for (auto &location : locations) {
		jcv_point point;
		point.x = location.x;
		point.y = location.y;
		points.push_back(point);
	}

	jcv_point dimensions;
	dimensions.x = (jcv_real)width;
	dimensions.y = (jcv_real)height;
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

	//struct cell *cells = new struct cell[diagram.numsites];
	cells.resize(diagram.numsites);
	// get each cell
	for (int i = 0; i < diagram.numsites; i++) {
		const jcv_site *site = &sites[i];
		struct cell cell;
		jcv_point p = site->p;
		cell.center = glm::vec2(p.x, p.y);
		cell.index = site->index;
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

	corners.resize(diagram.internal->numvertices);

	const jcv_vertex *vertex = jcv_diagram_get_vertices(&diagram);
	while (vertex) {
		struct corner c;
		c.index = vertex->index;
		c.position = glm::vec2(vertex->pos.x, vertex->pos.y);
		jcv_vertex_edge *edges = vertex->edges;
		while (edges) {
			jcv_vertex *neighbor = edges->neighbor;
			c.adjacent.push_back(&corners[neighbor->index]);
			edges = edges->next;
		}
		corners[vertex->index] = c;
		vertex = jcv_diagram_get_next_vertex(vertex);
	}

	// get corner and cell duality
	for (int i = 0; i < diagram.numsites; i++) { 
		const jcv_site *site = &sites[i];
		const jcv_graphedge *edge = site->edges;
		while (edge) {
			const jcv_altered_edge *altered = get_altered_edge(edge);
			jcv_vertex *a = altered->vertices[0];
			cells[site->index].corners.push_back(&corners[a->index]);
			jcv_vertex *b = altered->vertices[1];
			cells[site->index].corners.push_back(&corners[b->index]);

			edge = edge->next;
		}
	}

	// remove duplicates and add duality
	for (auto &cell : cells) {
		prune_corners(cell.corners);
		for (auto &corner : cell.corners) {
			corners[corner->index].cells.push_back(&cells[cell.index]);
		}
	}

	jcv_diagram_free(&diagram);
}
