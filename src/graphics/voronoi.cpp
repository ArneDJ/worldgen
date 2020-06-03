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

#define SEA_LEVEL 0.5
#define MOUNTAIN_LEVEL 0.7
#define NSITES 256*256
#define MAX_SITES 100


// Remaps the point from the input space to image space
static inline jcv_point remap(const jcv_point* pt, const jcv_point* min, const jcv_point* max, const jcv_point* scale)
{
    jcv_point p;
    p.x = (pt->x - min->x)/(max->x - min->x) * scale->x;
    p.y = (pt->y - min->y)/(max->y - min->y) * scale->y;
    return p;
}

void relax_points(const jcv_diagram* diagram, std::vector<jcv_point> &points)
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

struct delaunay {
	const jcv_site *a;
	const jcv_site *b;
	const jcv_site *c;
};

struct corner {
	glm::vec2 vertex;
	std::vector<struct corner*> adjacent;
};

struct border {
	//glm::vec2 a;
	//glm::vec2 b;
	const jcv_edge *edge;
	std::vector<const jcv_edge*> neighbors;
};

struct mycell {
	glm::vec2 center;
	std::vector<glm::vec4> borders;
	std::vector<struct mycell*> neighbors;
	// std::vector<struct corner*> corners;
};

void gen_cells(struct byteimage *image)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, image->width);

	std::vector<jcv_point> points;

	for (int i = 0; i < MAX_SITES; i++) {
		jcv_point point;
		point.x = dis(gen);
		point.y = dis(gen);
		int x = point.x;
		int y = point.y;
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

	unsigned char sitecolor[3] = {255, 255, 255};
	unsigned char linecolor[3] = {255, 255, 255};
	unsigned char delacolor[3] = {255, 0, 0};
	unsigned char blue[3] = {0, 0, 255};
	unsigned char white[3] = {255, 255, 255};

	struct mycell *cells = new struct mycell[diagram.numsites];
	// get each cell
	for (int i = 0; i < diagram.numsites; i++) {
		const jcv_site *site = &sites[i];
		struct mycell cell;
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
			const jcv_site *neighbor;
			 if (edge->edge->sites[0] == site) {
			 	neighbor = edge->edge->sites[1];
			 } else {
			 	neighbor = edge->edge->sites[0];
			 }

			 if (neighbor != nullptr) {
				cells[site->index].neighbors.push_back(&cells[neighbor->index]);
			 }

			 edge = edge->next;
		}
	}

	mycell cell = cells[28];
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

	// generate the vertices and vertex_edges
	jcv_diagram_generate_vertices(&diagram);

	{
	const jcv_vertex *vertex = jcv_diagram_get_vertices(&diagram);
	while (vertex) {
		plot((int)vertex->pos.x, (int)vertex->pos.y, image->data, image->width, image->height, image->nchannels, blue);
		jcv_vertex_edge *edges = vertex->edges; // The half edges owned by the vertex
		while (edges) {
			jcv_vertex *neighbor = edges->neighbor;
			draw_line(vertex->pos.x, vertex->pos.y, neighbor->pos.x, neighbor->pos.y, image->data, image->width, image->height, image->nchannels, white);
			edges = edges->next;
		}
		vertex = jcv_diagram_get_next_vertex(vertex);
	}
	}

	{
	const jcv_vertex *vertex = jcv_diagram_get_vertices(&diagram);
	while (vertex) {
		plot((int)vertex->pos.x, (int)vertex->pos.y, image->data, image->width, image->height, image->nchannels, blue);
		vertex = jcv_diagram_get_next_vertex(vertex);
	}
	}

	delete [] cells;
	jcv_diagram_free(&diagram);
}

/*
void do_voronoi(struct byteimage *image, const struct byteimage *heightimage)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(0, image->width);

	std::vector<jcv_point> points;

	const float ratio = float(image->width) / float(heightimage->width);
	for (int i = 0; i < NSITES; i++) {
		jcv_point point;
		point.x = dis(gen);
		point.y = dis(gen);
		int x = point.x / ratio;
		int y = point.y / ratio;
		points.push_back(point);
	}

	jcv_diagram diagram;
	memset(&diagram, 0, sizeof(jcv_diagram));
	jcv_diagram_generate(points.size(), points.data(), 0, 0, &diagram);

	std::vector<jcv_point> relaxed_points;
	relax_points(&diagram, relaxed_points);

	jcv_diagram_generate(relaxed_points.size(), relaxed_points.data(), 0, 0, &diagram);

	// fill the cells
	const jcv_site *cells = jcv_diagram_get_sites(&diagram);
	for (int i = 0; i < diagram.numsites; i++) {
		unsigned char rcolor[3];
		unsigned char basecolor = 100;
		rcolor[0] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[1] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		rcolor[2] = basecolor + (unsigned char)(rand() % (235 - basecolor));
		const jcv_site *site = &cells[i];
		const jcv_graphedge *e = site->edges;
		const jcv_point p = site->p;
		int x = p.x / ratio;
		int y = p.y / ratio;

		while (e) {
			draw_triangle(site->p.x, site->p.y, e->pos[0].x, e->pos[0].y, e->pos[1].x, e->pos[1].y, image->data, image->width, image->height, image->nchannels, rcolor);
			e = e->next;
		}
	}

	unsigned char linecolor[3] = {0, 0, 0};
	// If all you need are the edges
	const jcv_edge* edge = jcv_diagram_get_edges(&diagram);
	while( edge ) {
		draw_line(edge->pos[0].x, edge->pos[0].y, edge->pos[1].x, edge->pos[1].y, image->data, image->width, image->height, image->nchannels, linecolor);
		edge = jcv_diagram_get_next_edge(edge);
	}

	// plot the sites
	unsigned char sitecolor[3] = {0, 0, 0};
	const jcv_site *sites = jcv_diagram_get_sites(&diagram);
	for (int i = 0; i < diagram.numsites; i++) {
		const jcv_site *site = &sites[i];
		jcv_point p = site->p;
		plot((int)p.x, (int)p.y, image->data, image->width, image->height, image->nchannels, sitecolor);
	}

	jcv_diagram_free(&diagram);
}
*/
