#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <set>
#include <glm/glm.hpp>
#include <glm/vec3.hpp>

#define JC_VORONOI_IMPLEMENTATION
#include "extern/jc_voronoi.h"

#include "voronoi.h"

static void prune_vertices(std::vector<const struct vertex*> &v)
{
	auto end = v.end();
	for (auto it = v.begin(); it != end; ++it) {
		end = std::remove(it + 1, end, *it);
	}

	v.erase(end, v.end());
}

// Remaps the point from the input space to image space
static inline jcv_point remap(const jcv_point *pt, const jcv_point *min, const jcv_point *max, const jcv_point *scale)
{
	jcv_point p;
	p.x = (pt->x - min->x)/(max->x - min->x) * scale->x;
	p.y = (pt->y - min->y)/(max->y - min->y) * scale->y;

	return p;
}

static void relax_points(const jcv_diagram *diagram, std::vector<jcv_point> &points)
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

static void adapt_cells(const jcv_diagram *diagram, std::vector<struct cell> &cells)
{
	const jcv_site *sites = jcv_diagram_get_sites(diagram);

	// get each cell
	for (int i = 0; i < diagram->numsites; i++) {
		const jcv_site *site = &sites[i];
		struct cell cell;
		jcv_point p = site->p;
		cell.center = glm::vec2(p.x, p.y);
		cell.index = site->index;
		cells[site->index] = cell;
	}
	// get neighbor cells
	for (int i = 0; i < diagram->numsites; i++) { 
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
}

static void adapt_vertices(const jcv_diagram *diagram, std::vector<struct vertex> &vertices)
{
	const jcv_site *sites = jcv_diagram_get_sites(diagram);
	const jcv_vertex *vertex = jcv_diagram_get_vertices(diagram);
	while (vertex) {
		struct vertex c;
		c.index = vertex->index;
		c.position = glm::vec2(vertex->pos.x, vertex->pos.y);
		jcv_vertex_edge *edges = vertex->edges;
		while (edges) {
			jcv_vertex *neighbor = edges->neighbor;
			c.adjacent.push_back(&vertices[neighbor->index]);
			edges = edges->next;
		}
		vertices[vertex->index] = c;
		vertex = jcv_diagram_get_next_vertex(vertex);
	}
}

static void pair_duality(const jcv_diagram *diagram, std::vector<struct cell> &cells, std::vector<struct vertex> &vertices)
{
	const jcv_site *sites = jcv_diagram_get_sites(diagram);

	// get vertex and cell duality
	for (int i = 0; i < diagram->numsites; i++) { 
		const jcv_site *site = &sites[i];
		const jcv_graphedge *edge = site->edges;
		while (edge) {
			const jcv_altered_edge *altered = get_altered_edge(edge);
			jcv_vertex *a = altered->vertices[0];
			cells[site->index].vertices.push_back(&vertices[a->index]);
			jcv_vertex *b = altered->vertices[1];
			cells[site->index].vertices.push_back(&vertices[b->index]);

			edge = edge->next;
		}
	}
	// remove duplicates and add duality
	for (auto &cell : cells) {
		prune_vertices(cell.vertices);
		for (auto &vertex : cell.vertices) {
			vertices[vertex->index].cells.push_back(&cells[cell.index]);
		}
	}
}

static void adapt_edges(const jcv_diagram *diagram, std::vector<struct cell> &cells, std::vector<struct vertex> &vertices, std::vector<struct edge> &edges)
{
	const jcv_edge *jcedge = jcv_diagram_get_edges(diagram);
	while (jcedge) {
		struct edge e;
		if (jcedge->sites[0] != nullptr) {
			e.c0 = &cells[jcedge->sites[0]->index];
		}
		if (jcedge->sites[1] != nullptr) {
			e.c1 = &cells[jcedge->sites[1]->index];
		}

    		const jcv_altered_edge *alter = (jcv_altered_edge*)jcedge;
		e.v0 = &vertices[alter->vertices[0]->index];
		e.v1 = &vertices[alter->vertices[1]->index];

		edges.push_back(e);
		jcedge = jcv_diagram_get_next_edge(jcedge);
	}

	for (int i = 0; i < edges.size(); i++) {
		struct edge *e = &edges[i];
		e->index = i;
		if (e->c0 != nullptr) {
			cells[e->c0->index].edges.push_back(&edges[i]);
		}
		if (e->c1 != nullptr) {
			cells[e->c1->index].edges.push_back(&edges[i]);
		}
	}
}

void Voronoi::gen_diagram(std::vector<glm::vec2> &locations, glm::vec2 min, glm::vec2 max, bool relax)
{
	std::vector<jcv_point> points;
	for (auto &location : locations) {
		jcv_point point;
		point.x = location.x;
		point.y = location.y;
		points.push_back(point);
	}

	const jcv_rect rect = {
		{min.x, min.y},
		{max.x, max.y},
	};
	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(points.size(), points.data(), &rect, &diagram);

	if (relax == true) {
		std::vector<jcv_point> relaxed_points;
		relax_points(&diagram, relaxed_points);
		jcv_diagram_generate(relaxed_points.size(), relaxed_points.data(), &rect, &diagram);
	}

	jcv_diagram_generate_vertices(&diagram);

	cells.resize(diagram.numsites);
	vertices.resize(diagram.internal->numvertices);

	adapt_cells(&diagram, cells);

	adapt_vertices(&diagram, vertices);

	pair_duality(&diagram, cells, vertices);

	adapt_edges(&diagram, cells, vertices, edges);

	jcv_diagram_free(&diagram);
}
